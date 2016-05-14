/*
 * Copyright (C) 2012 The CyanogenMod Project
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

package com.cyanogenmod.settings.device.preferences;

import android.content.Context;
import android.util.AttributeSet;

import com.cyanogenmod.settings.device.DisplaySettings;

import org.namelessrom.settings.device.R;

public class mDNIeMode extends mDNIeBasePreference {

    @Override public int getPathArrayResId() {
        return R.array.mdnie_mode_sysfs_paths;
    }

    public mDNIeMode(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public static void restore(Context context) {
        mDNIeBasePreference.restore(context, DisplaySettings.KEY_MDNIE_MODE, R.array.mdnie_mode_sysfs_paths);
    }

}
