#include "LuaTimer.h"
#include "LuaUtils.h"
#include "Pi.h"

void LuaTimer::Tick()
{
	lua_State *l = Pi::luaManager.GetLuaState();

	LUA_DEBUG_START(l);

	lua_getfield(l, LUA_REGISTRYINDEX, "PiTimerCallbacks");
	if (lua_isnil(l, -1)) {
		lua_pop(l, 1);
		LUA_DEBUG_END(l, 0);
		return;
	}
	assert(lua_istable(l, -1));

	double now = Pi::GetGameTime();

	lua_pushnil(l);
	while (lua_next(l, -2)) {
		assert(lua_istable(l, -1));

		lua_getfield(l, -1, "at");
		double at = lua_tonumber(l, -1);
		lua_pop(l, 1);

		if (at <= now) {
			lua_getfield(l, -1, "callback");
			lua_call(l, 0, 1);
			bool cancel = lua_toboolean(l, -1);
			lua_pop(l, 1);

			lua_getfield(l, -1, "every");
			if (lua_isnil(l, -1) || cancel) {
				lua_pop(l, 1);

				lua_pushvalue(l, -2);
				lua_pushnil(l);
				lua_settable(l, -5);
			}
			else {
				double every = lua_tonumber(l, -1);
				lua_pop(l, 1);

				pi_lua_settable(l, "at", Pi::GetGameTime() + every);
			}
		}

		lua_pop(l, 1);
	}
	lua_pop(l, 1);

	LUA_DEBUG_END(l, 0);
}

/*
 * Class: Timer
 *
 * A class to invoke functions at specific times.
 *
 * The <Timer> class provides a facility whereby scripts can request that a
 * function be called at a given time, or regularly.
 *
 * Pioneer provides a single <Timer> object to the Lua environment. It resides
 * in the global namespace and is simply called Timer.
 *
 * The <Timer> is bound to the game clock, not the OS (real time) clock. The
 * game clock is subject to time acceleration. As such, timer triggers will
 * not necessarily occur at the exact time you request but can arrive seconds,
 * minutes or even hours after the requested time (game time).
 *
 * Because timer functions are called outside of the normal event model, it is
 * possible that game objects no longer exist. Consider this example:
 *
 * > local enemy = Space.SpawnShipNear("Eagle Long Range Fighter", Game.player, 20, 20)
 * > UI.ImportantMessage(enemy:GetLabel(), "You have 20 seconds to surrender or you will be destroyed.")
 * > Timer:CallAt(Game.time+20, function ()
 * >     UI.ImportantMessage(enemy:GetLabel(), "You were warned. Prepare to die!")
 * >     enemy:Kill(Game.player)
 * > end)
 *
 * This works exactly as you'd expect: 20 seconds after the threat message is
 * sent, the enemy comes to life and attacks the player. If however the player
 * chooses to avoid the battle by hyperspacing away, the enemy ship is
 * destroyed by the game engine. In that case, the "enemy" object held by the
 * script is a shell, and any attempt to use it will be greeted by a Lua error.
 *
 * To protect against this, you should call <Object.exists> to confirm that the
 * underlying object exists before trying to use it.
 */

static void _finish_timer_create(lua_State *l)
{
	lua_pushstring(l, "callback");
	lua_pushvalue(l, 3);
	lua_settable(l, -3);

	lua_getfield(l, LUA_REGISTRYINDEX, "PiTimerCallbacks");
	if (lua_isnil(l, -1)) {
		lua_pop(l, 1);
		lua_newtable(l);
		lua_pushvalue(l, -1);
		lua_setfield(l, LUA_REGISTRYINDEX, "PiTimerCallbacks");
	}

	lua_insert(l, -2);
	lua_pushinteger(l, lua_objlen(l, -2) + 1);
	lua_insert(l, -2);
	lua_settable(l, -3);

	lua_pop(l, 1);
}

/*
 * Method: CallAt
 *
 * Request that a function be called at a specific game time.
 *
 * > Timer:CallAt(time, function)
 *
 * Time acceleration may cause the function to be called long after the desired
 * time has passed.
 *
 * Parameters:
 *
 *   time - the absolute game time to call the function at. This will usually
 *          be created by adding some small amount to <Game.time>.
 *
 *   function - the function to call. Takes no arguments and returns nothing.
 *
 * Example:
 *
 * > Timer:CallAt(Game.time+30, function ()
 * >     UI.Message("Special offer expired, sorry.")
 * > end)
 *
 * Availability:
 *
 *   alpha 10
 *
 * Status:
 *
 *   stable
 */
static int l_timer_call_at(lua_State *l)
{
	double at = luaL_checknumber(l, 2);
	if (!lua_isfunction(l, 3))
		luaL_typerror(l, 3, lua_typename(l, LUA_TFUNCTION));
	
	if (at <= Pi::GetGameTime())
		luaL_error(l, "Specified time is in the past");
	
	LUA_DEBUG_START(l);

	lua_newtable(l);
	pi_lua_settable(l, "at", at);

	_finish_timer_create(l);

	LUA_DEBUG_END(l, 0);

	return 0;
}

/*
 * Method: CallEvery
 *
 * Request that a function be called over at over at a regular interval.
 *
 * > Timer:CallEvery(interval, function)
 *
 * Since the <Timer> system is locked to the game time, time acceleration may
 * cause the function to be called more frequently than the corresponding
 * number of real-time seconds. Even under time acceleration, the function
 * will never called more than once per real-time second.
 *
 * If the called function returns a false value (as is the default for Lua
 * when no return value is specified), the timer will continue to be triggered
 * after each interval. To request that no further timer events be fired, the
 * function should explicitly return a true value.
 *
 * Parameters:
 *
 *   time - the interval between calls to the function, in seconds
 *
 *   function - the function to call. Returns false to continue receiving
 *              calls after the next interval, or true to cancel the timer.
 *
 * Example:
 *
 * > -- dump fuel every two seconds until none left
 * > Timer:CallEvery(2, function ()
 * >     local did_dump = Game.player:Jettison(Equip.Type.HYDROGEN)
 * >     return not did_dump
 * > end)
 *
 * Availability:
 *
 *   alpha 10
 *
 * Status:
 *
 *   stable
 */
static int l_timer_call_every(lua_State *l)
{
	double every = luaL_checknumber(l, 2);
	if (!lua_isfunction(l, 3))
		luaL_typerror(l, 3, lua_typename(l, LUA_TFUNCTION));
	
	if (every <= 0)
		luaL_error(l, "Specified interval must be greater than zero");
	
	LUA_DEBUG_START(l);

	lua_newtable(l);
	pi_lua_settable(l, "every", every);
	pi_lua_settable(l, "at", Pi::GetGameTime() + every);

	_finish_timer_create(l);

	LUA_DEBUG_END(l, 0);

	return 0;
}

template <> const char *LuaObject<LuaTimer>::s_type = "Timer";

template <> void LuaObject<LuaTimer>::RegisterClass()
{
	static const luaL_reg l_methods[] = {
		{ "CallAt",    l_timer_call_at    },
		{ "CallEvery", l_timer_call_every },
		{ 0, 0 }
	};

	LuaObjectBase::CreateClass(s_type, NULL, l_methods, NULL, NULL);
}