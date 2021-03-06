// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "utils.h"
#include "libs.h"
#include "gameconsts.h"
#include "StringF.h"
#include "gui/Gui.h"
#include "Lang.h"
#include "FileSystem.h"
#include "PngWriter.h"
#include <sstream>
#include <cmath>
#include <cstdio>

std::string format_money(double cents, bool showCents){
	char *end;                                   // for  error checking
	size_t groupDigits = strtol(Lang::NUMBER_GROUP_NUM, &end, 10);
	assert(*end == 0);

	double money = showCents ? 0.01*cents : roundf(0.01*cents);

	const char *format = (money < 0) ? "-$%.2f" : "$%.2f";
	char buf[64];
	snprintf(buf, sizeof(buf), format, std::abs(money));
	std::string result(buf);

	size_t pos = result.find_first_of('.');      // pos to decimal point

	if(showCents)                                // replace decimal point
		result.replace(pos, 1, Lang::NUMBER_DECIMAL_POINT);
	else                                         // or just remove frac. part
		result.erase(result.begin() + pos, result.end());

	size_t groupMin = strtol(Lang::NUMBER_GROUP_MIN, &end, 10);
	assert(*end == 0);

	if(groupDigits != 0 && std::abs(money) >= groupMin){

		std::string groupSep = std::string(Lang::NUMBER_GROUP_SEP) == " " ?
			"\u00a0" : Lang::NUMBER_GROUP_SEP;     // space should be fixed space

		size_t skip = (money < 0) ? 2 : 1;        // compensate for "$" or "-$"
		while(pos - skip > groupDigits){          // insert thousand seperator
			pos = pos - groupDigits;
			result.insert(pos, groupSep);
		}
	}
	return result;
}

class timedate {
public:
	timedate() : hour(0), minute(0), second(0), day(0), month(0), year(3200) {}
	timedate(int stamp) { *this = stamp; }
	timedate &operator=(int stamp);
	std::string fmt_time_date();
	std::string fmt_date();
private:
	int hour, minute, second, day, month, year;

	static const char * const months[12];
	static const unsigned char days[2][12];
};

const char * const timedate::months[] = {
	Lang::MONTH_JAN,
	Lang::MONTH_FEB,
	Lang::MONTH_MAR,
	Lang::MONTH_APR,
	Lang::MONTH_MAY,
	Lang::MONTH_JUN,
	Lang::MONTH_JUL,
	Lang::MONTH_AUG,
	Lang::MONTH_SEP,
	Lang::MONTH_OCT,
	Lang::MONTH_NOV,
	Lang::MONTH_DEC
};

const unsigned char timedate::days[2][12] = {
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

timedate &timedate::operator=(int stamp)
{
	int i = int(stamp) % 86400;

	hour   = (i / 3600 + 24)%24; i %= 3600;
	minute = (i /   60 + 60)%60; i %=   60;
	second = (i+60)%60;

	i = int(stamp) / 86400 + 1168410 - ((stamp < 0)?1:0); // days since "year 0"

	int n400 = i / 146097; i %= 146097;
	int n100 = i /  36524; i %=  36524;
	int n4   = i /   1461; i %=   1461;
	int n1   = i /    365;

	year = n1 + n4 * 4 + n100 * 100 + n400 * 400 + !(n100 == 4 || n1 == 4);
	day = i % 365 + (n100 == 4 || n1 == 4) * 365;
	int leap = (year % 4 == 0 && year % 100) || (year % 400 == 0);

	month = 0;
	while (day >= days[leap][month])
		day -= days[leap][month++];

	return *this;
}

std::string timedate::fmt_time_date()
{
	char buf[32];
	snprintf(buf, sizeof (buf), "%02d:%02d:%02d %d %s %d",
	         hour, minute, second, day + 1, months[month], year);
	return buf;
}

std::string timedate::fmt_date()
{
	char buf[16];
	snprintf(buf, sizeof (buf), "%d %s %d",
	         day + 1, months[month], year);
	return buf;
}

std::string format_date(double t)
{
	timedate stamp = int(t);
	return stamp.fmt_time_date();
}

std::string format_date_only(double t)
{
	timedate stamp = int(t);
	return stamp.fmt_date();
}

std::string string_join(std::vector<std::string> &v, std::string sep)
{
	std::vector<std::string>::iterator i = v.begin();
	std::string out;

	while (i != v.end()) {
		out += *i;
		++i;
		if (i != v.end()) out += sep;
	}
	return out;
}

void Error(const char *format, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	Output("error: %s\n", buf);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Pioneer error", buf, 0);

	exit(1);
}

void Warning(const char *format, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	Output("warning: %s\n", buf);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Pioneer warning", buf, 0);
}

void Output(const char *format, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	fputs(buf, stderr);
}
std::string format_number(double number)
{
	//Put the last numeros to zero to improve readness of the number.
	//Ex: 45426 -> 45400 
	std::ostringstream ss;
	ss.setf(std::ios::fixed, std::ios::floatfield);
	ss.precision(0);
	if (number < 100000.0) {
		if (number < 10000.0) {
			if (number < 100.0) {
				ss << number;
			} else {
				ss << number * 0.1 << "0";
			}
		} else {
			ss << number * 0.01 << "00";
		}
	} else {
		ss << number * 0.001 << "000";
	}
	return ss.str();
}
std::string format_distance(double dist, int precision) // en m
{
	std::ostringstream ss;
	ss.setf(std::ios::fixed, std::ios::floatfield);
	if (dist < 10000) { // < 10 000 m -> display m
		ss.precision(0);
		ss << format_number(dist) << " m";
	} else { // >= 100 000 m -> display km
		ss.precision(precision);
		if (dist < AU) {	
			double kmDist = dist * 0.001;
			if (kmDist < 1000000.0) { // < 100 000 km -> 45600 km
				ss.precision(0);
				ss << format_number(kmDist) << " km";				
			} else { // 1 AU <= dist < 1 M.km -> 1.45 M.km
				ss << (kmDist*0.000001) << " M.km";
			}
		} else {
			ss << (dist/AU) << " AU";
		}
	}
	return ss.str();
}


std::string format_speed(double speed) //m/s
{
	// 3 km/h 
	// 55 km/h
	// 100 km/h
	// 450 km/h
	// 5600 km/h
	// 11200 km/h
	// 36500 km/h = 10km/s = 10000m/s
	// 13 km/s
	// 320 km/s
	// 3500 km/s
	// 11800 km/s
	// 54900 km/s
	std::ostringstream ss;
	ss.setf(std::ios::fixed, std::ios::floatfield);
	std::string speedFormat = ""; 
	double mySpeed = 0.0;

	//km/s or km/h
	if (speed > 10000.0) {
		speedFormat = " km/s";
		mySpeed = speed * 0.001;
	} else {
		speedFormat = " km/h";
		mySpeed = speed * 3.6;
	}
	ss.precision(0);
	ss << format_number(mySpeed) << speedFormat;
	
	return ss.str();
}

void Screendump(const char* destFile, const int width, const int height)
{
	const std::string dir = "screenshots";
	FileSystem::userFiles.MakeDirectory(dir);
	const std::string fname = FileSystem::JoinPathBelow(dir, destFile);

	// pad rows to 4 bytes, which is the default row alignment for OpenGL
	const int stride = (3*width + 3) & ~3;

	std::vector<Uint8> pixel_data(stride * height);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 4); // never trust defaults
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, &pixel_data[0]);
	glFinish();

	write_png(FileSystem::userFiles, fname, &pixel_data[0], width, height, stride, 3);

	Output("Screenshot %s saved\n", fname.c_str());
}

// strcasestr() adapted from gnulib
// (c) 2005 FSF. GPL2+

#define TOLOWER(c) (isupper(c) ? tolower(c) : (c))

const char *pi_strcasestr (const char *haystack, const char *needle)
{
	if (!*needle)
		return haystack;

	// cache the first character for speed
	char b = TOLOWER(*needle);

	needle++;
	for (;; haystack++) {
		if (!*haystack)
			return 0;

		if (TOLOWER(*haystack) == b) {
			const char *rhaystack = haystack + 1;
			const char *rneedle = needle;

			for (;; rhaystack++, rneedle++) {
				if (!*rneedle)
					return haystack;

				if (!*rhaystack)
					return 0;

				if (TOLOWER(*rhaystack) != TOLOWER(*rneedle))
					break;
			}
		}
	}
}

static const int HEXDUMP_CHUNK = 16;
void hexdump(const unsigned char *buf, int len)
{
	int count;

	for (int i = 0; i < len; i += HEXDUMP_CHUNK) {
		Output("0x%06x  ", i);

		count = ((len-i) > HEXDUMP_CHUNK ? HEXDUMP_CHUNK : len-i);

		for (int j = 0; j < count; j++) {
			if (j == HEXDUMP_CHUNK/2) Output(" ");
			Output("%02x ", buf[i+j]);
		}

		for (int j = count; j < HEXDUMP_CHUNK; j++) {
			if (j == HEXDUMP_CHUNK/2) Output(" ");
			Output("   ");
		}

		Output(" ");

		for (int j = 0; j < count; j++)
			Output("%c", isprint(buf[i+j]) ? buf[i+j] : '.');

		Output("\n");
	}
}
