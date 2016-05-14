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

package com.cyanogenmod.settings.device;

import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.text.TextUtils;

import com.cyanogenmod.settings.device.preferences.mDNIeBasePreference;
import com.cyanogenmod.settings.device.preferences.mDNIeMode;
import com.cyanogenmod.settings.device.preferences.mDNIeNegative;
import com.cyanogenmod.settings.device.preferences.mDNIeScenario;

import org.namelessrom.settings.device.R;

public class ScreenPreferenceFragment extends PreferenceFragment {
    private mDNIeScenario mDNIeScenario;
    private mDNIeMode mDNIeMode;
    private mDNIeNegative mDNIeNegative;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.screen_preferences);

        mDNIeScenario = (mDNIeScenario) findPreference(DisplaySettings.KEY_MDNIE_SCENARIO);
        mDNIeScenario.setEnabled(isSupported(mDNIeScenario.getPathArrayResId()));

        mDNIeMode = (mDNIeMode) findPreference(DisplaySettings.KEY_MDNIE_MODE);
        mDNIeMode.setEnabled(isSupported(mDNIeMode.getPathArrayResId()));

        mDNIeNegative = (mDNIeNegative) findPreference(DisplaySettings.KEY_MDNIE_NEGATIVE);
        mDNIeNegative.setEnabled(isSupported(mDNIeNegative.getPathArrayResId()));
    }

    private boolean isSupported(int pathArrayResId) {
        final String[] filePaths = getResources().getStringArray(pathArrayResId);
        final String filePath = mDNIeBasePreference.isSupported(filePaths);
        return !TextUtils.isEmpty(filePath);
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {
        return true;
    }
}
