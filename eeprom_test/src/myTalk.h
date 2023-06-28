//
// MIT License
//
// Copyright (C) 2021 - Dave Gutz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef _MY_TALK_H
#define _MY_TALK_H

enum urgency {INCOMING, ASAP, SOON, QUEUE, NEW};
typedef enum urgency urgency;


double decimalTime(unsigned long *current_time, char* tempStr, unsigned long now, unsigned long millis_flip);
String time_long_2_str(const unsigned long current_time, char *tempStr);
String tryExtractString(String str, const char* start, const char* end);
void asap(void);
void chat(void);
void chit(const String cmd, const enum urgency when);
void clear_queues(void);
void finish_request(void);
void get_string(String *source);
void talk();
void talkH(); // Help

#endif
