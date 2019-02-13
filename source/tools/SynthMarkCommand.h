/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef ANDROID_SYNTHMARK_COMMAND_H
#define ANDROID_SYNTHMARK_COMMAND_H

#include <cstdio>

/**
 * This function will be called by int main().
 *
 * It allows the guts of the synthmark app to be visible in the AndroidStudio project
 *
 */
int synthmark_command_main(int argc, char **argv);

#endif //ANDROID_SYNTHMARK_COMMAND_H
