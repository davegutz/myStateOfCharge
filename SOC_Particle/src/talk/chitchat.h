//
// MIT License
//
// Copyright (C) 2023 - Dave Gutz
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

#ifndef _CHITCHAT
#define _CHITCHAT

enum urgency {INCOMING, CONTROL, ASAP, SOON, QUEUE, NEW, LAST};
typedef enum urgency urgency;

class BatteryMonitor;
class Sensors;

void benign_zero(BatteryMonitor *Mon, Sensors *Sen);
void chat();
void chatter();
void chit(const String cmd, const enum urgency when);
void chitter(const boolean chitchat, BatteryMonitor *Mon, Sensors *Sen);
String chit_nibble_ctl();
String chit_nibble_inp();
void cmd_echo(urgency request);
urgency chit_classify_nibble(String *nibble);
void describe(BatteryMonitor *Mon, Sensors *Sen);

#endif
