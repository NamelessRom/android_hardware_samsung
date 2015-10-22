/*
 * Copyright (C) 2015 The NamelessRom Project
 *
 * Not a Contribution, Apache license notifications and license are retained
 * for attribution purposes only.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HWCOMPOSER_UTILS_H
#define HWCOMPOSER_UTILS_H

#define DEBUG 1
#define DEBUG_SPAMMY 0
#define DEBUG_VSYNC 1

#ifdef ALOGD
#undef ALOGD
#endif
#define ALOGD(...) if (DEBUG) ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))

#endif // HWCOMPOSER_UTILS_H
