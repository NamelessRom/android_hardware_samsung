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
import android.content.SharedPreferences;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.util.AttributeSet;

import com.cyanogenmod.settings.device.Utils;

public abstract class mDNIeBasePreference extends ListPreference implements OnPreferenceChangeListener {
    private String mFile;

    /** Override to implement multiple paths */
    public abstract int getPathArrayResId();

    public mDNIeBasePreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        final String[] paths = context.getResources().getStringArray(getPathArrayResId());
        mFile = isSupported(paths);

        this.setOnPreferenceChangeListener(this);
    }

    public static String isSupported(String[] filePaths) {
        for (final String filePath : filePaths) {
            if (Utils.fileExists(filePath)) {
                return filePath;
            }
        }
        return null;
    }

    public static String isSupported(String filePath) {
        if (Utils.fileExists(filePath)) {
            return filePath;
        }
        return null;
    }

    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (TextUtils.isEmpty(mFile)) {
            return false;
        }
        Utils.writeValue(mFile, (String) newValue);
        return true;
    }

    /**
     * Restore mdnie user mode setting from SharedPreferences. (Write to kernel.)
     *
     * @param context       The context to read the SharedPreferences from
     * @param key           The key of the shared preference
     * @param filePathResId The resource id of the string array containing the sysfs paths
     */
    /* package */ static void restore(Context context, String key, int filePathResId) {
        final String[] filePaths = context.getResources().getStringArray(filePathResId);
        final String supportedFile = isSupported(filePaths);
        if (TextUtils.isEmpty(supportedFile)) {
            return;
        }

        SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        Utils.writeValue(supportedFile, sharedPrefs.getString(key, "0"));
    }

}
