/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// INTERNAL USE ONLY: Do NOT use for any other purpose.
//

#include "qalsaaudiodeviceinfo_p.h"

#include <alsa/version.h>

QT_BEGIN_NAMESPACE

QAlsaAudioDeviceInfo::QAlsaAudioDeviceInfo(const QByteArray &dev, const QString &description, QAudio::Mode mode)
    : QAudioDeviceInfoPrivate(dev, mode)
    , m_description(description)
{
    handle = 0;

    this->mode = mode;

    checkSurround();
}

QAlsaAudioDeviceInfo::~QAlsaAudioDeviceInfo()
{
    close();
}

bool QAlsaAudioDeviceInfo::isFormatSupported(const QAudioFormat& format) const
{
    return testSettings(format);
}

QAudioFormat QAlsaAudioDeviceInfo::preferredFormat() const
{
    QAudioFormat nearest;
    if(mode == QAudio::AudioOutput) {
        nearest.setSampleRate(44100);
        nearest.setChannelCount(2);
        nearest.setByteOrder(QAudioFormat::LittleEndian);
        nearest.setSampleType(QAudioFormat::SignedInt);
        nearest.setSampleSize(16);
        nearest.setCodec(QLatin1String("audio/x-raw"));
    } else {
        nearest.setSampleRate(8000);
        nearest.setChannelCount(1);
        nearest.setSampleType(QAudioFormat::UnSignedInt);
        nearest.setSampleSize(8);
        nearest.setCodec(QLatin1String("audio/x-raw"));
        if(!testSettings(nearest)) {
            nearest.setChannelCount(2);
            nearest.setSampleSize(16);
            nearest.setSampleType(QAudioFormat::SignedInt);
        }
    }
    return nearest;
}

QStringList QAlsaAudioDeviceInfo::supportedCodecs() const
{
    updateLists();
    return codecz;
}

QList<int> QAlsaAudioDeviceInfo::supportedSampleRates() const
{
    updateLists();
    return sampleRatez;
}

QList<int> QAlsaAudioDeviceInfo::supportedChannelCounts() const
{
    updateLists();
    return channelz;
}

QList<int> QAlsaAudioDeviceInfo::supportedSampleSizes() const
{
    updateLists();
    return sizez;
}

QList<QAudioFormat::Endian> QAlsaAudioDeviceInfo::supportedByteOrders() const
{
    updateLists();
    return byteOrderz;
}

QList<QAudioFormat::SampleType> QAlsaAudioDeviceInfo::supportedSampleTypes() const
{
    updateLists();
    return typez;
}

QByteArray QAlsaAudioDeviceInfo::defaultDevice(QAudio::Mode)
{
    return "default";
}

bool QAlsaAudioDeviceInfo::open() const
{
    int err = 0;

    if(mode == QAudio::AudioOutput) {
        err = snd_pcm_open(&handle, id.constData(), SND_PCM_STREAM_PLAYBACK,0);
    } else {
        err = snd_pcm_open(&handle, id.constData(), SND_PCM_STREAM_CAPTURE,0);
    }
    if(err < 0) {
        handle = 0;
        return false;
    }
    return true;
}

void QAlsaAudioDeviceInfo::close() const
{
    if(handle)
        snd_pcm_close(handle);
    handle = 0;
}

bool QAlsaAudioDeviceInfo::testSettings(const QAudioFormat& format) const
{
    // Set nearest to closest settings that do work.
    // See if what is in settings will work (return value).
    int err = -1;
    snd_pcm_t* pcmHandle;
    snd_pcm_hw_params_t *params;

    snd_pcm_stream_t stream = mode == QAudio::AudioOutput
                            ? SND_PCM_STREAM_PLAYBACK : SND_PCM_STREAM_CAPTURE;

    if (snd_pcm_open(&pcmHandle, id.constData(), stream, 0) < 0)
        return false;

    snd_pcm_nonblock(pcmHandle, 0);
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcmHandle, params);

    // set the values!
    snd_pcm_hw_params_set_channels(pcmHandle, params, format.channelCount());
    snd_pcm_hw_params_set_rate(pcmHandle, params, format.sampleRate(), 0);

    snd_pcm_format_t pcmFormat = SND_PCM_FORMAT_UNKNOWN;
    switch (format.sampleSize()) {
    case 8:
        if (format.sampleType() == QAudioFormat::SignedInt)
            pcmFormat = SND_PCM_FORMAT_S8;
        else if (format.sampleType() == QAudioFormat::UnSignedInt)
            pcmFormat = SND_PCM_FORMAT_U8;
        break;
    case 16:
        if (format.sampleType() == QAudioFormat::SignedInt) {
            pcmFormat = format.byteOrder() == QAudioFormat::LittleEndian
                      ? SND_PCM_FORMAT_S16_LE : SND_PCM_FORMAT_S16_BE;
        } else if (format.sampleType() == QAudioFormat::UnSignedInt) {
            pcmFormat = format.byteOrder() == QAudioFormat::LittleEndian
                      ? SND_PCM_FORMAT_U16_LE : SND_PCM_FORMAT_U16_BE;
        }
        break;
    case 32:
        if (format.sampleType() == QAudioFormat::SignedInt) {
            pcmFormat = format.byteOrder() == QAudioFormat::LittleEndian
                      ? SND_PCM_FORMAT_S32_LE : SND_PCM_FORMAT_S32_BE;
        } else if (format.sampleType() == QAudioFormat::UnSignedInt) {
            pcmFormat = format.byteOrder() == QAudioFormat::LittleEndian
                      ? SND_PCM_FORMAT_U32_LE : SND_PCM_FORMAT_U32_BE;
        } else if (format.sampleType() == QAudioFormat::Float) {
            pcmFormat = format.byteOrder() == QAudioFormat::LittleEndian
                      ? SND_PCM_FORMAT_FLOAT_LE : SND_PCM_FORMAT_FLOAT_BE;
        }
    }

    if (pcmFormat != SND_PCM_FORMAT_UNKNOWN)
        err = snd_pcm_hw_params_set_format(pcmHandle, params, pcmFormat);

    // For now, just accept only audio/x-raw codec
    if (!format.codec().startsWith(QLatin1String("audio/x-raw")))
        err = -1;

    if (err >= 0 && format.channelCount() != -1) {
        err = snd_pcm_hw_params_test_channels(pcmHandle, params, format.channelCount());
        if (err >= 0)
            err = snd_pcm_hw_params_set_channels(pcmHandle, params, format.channelCount());
    }

    if (err >= 0 && format.sampleRate() != -1) {
        err = snd_pcm_hw_params_test_rate(pcmHandle, params, format.sampleRate(), 0);
        if (err >= 0)
            err = snd_pcm_hw_params_set_rate(pcmHandle, params, format.sampleRate(), 0);
    }

    if (err >= 0 && pcmFormat != SND_PCM_FORMAT_UNKNOWN)
        err = snd_pcm_hw_params_set_format(pcmHandle, params, pcmFormat);

    if (err >= 0)
        err = snd_pcm_hw_params(pcmHandle, params);

    snd_pcm_close(pcmHandle);

    return (err == 0);
}

void QAlsaAudioDeviceInfo::updateLists() const
{
    // redo all lists based on current settings
    sampleRatez.clear();
    channelz.clear();
    sizez.clear();
    byteOrderz.clear();
    typez.clear();
    codecz.clear();

    if(!handle)
        open();

    if(!handle)
        return;

    for(int i=0; i<(int)MAX_SAMPLE_RATES; i++) {
        //if(snd_pcm_hw_params_test_rate(handle, params, SAMPLE_RATES[i], dir) == 0)
        sampleRatez.append(SAMPLE_RATES[i]);
    }
    channelz.append(1);
    channelz.append(2);
    if (surround40) channelz.append(4);
    if (surround51) channelz.append(6);
    if (surround71) channelz.append(8);
    sizez.append(8);
    sizez.append(16);
    sizez.append(32);
    byteOrderz.append(QAudioFormat::LittleEndian);
    byteOrderz.append(QAudioFormat::BigEndian);
    typez.append(QAudioFormat::SignedInt);
    typez.append(QAudioFormat::UnSignedInt);
    typez.append(QAudioFormat::Float);
    codecz.append(QLatin1String("audio/x-raw"));
    close();
}

void QAlsaAudioDeviceInfo::checkSurround()
{
    surround40 = false;
    surround51 = false;
    surround71 = false;

    void **hints, **n;
    char *name, *descr, *io;

    if(snd_device_name_hint(-1, "pcm", &hints) < 0)
        return;

    n = hints;

    while (*n != NULL) {
        name = snd_device_name_get_hint(*n, "NAME");
        descr = snd_device_name_get_hint(*n, "DESC");
        io = snd_device_name_get_hint(*n, "IOID");
        if((name != NULL) && (descr != NULL)) {
            QString deviceName = QLatin1String(name);
            if (mode == QAudio::AudioOutput) {
                if(deviceName.contains(QLatin1String("surround40")))
                    surround40 = true;
                if(deviceName.contains(QLatin1String("surround51")))
                    surround51 = true;
                if(deviceName.contains(QLatin1String("surround71")))
                    surround71 = true;
            }
        }
        if(name != NULL)
            free(name);
        if(descr != NULL)
            free(descr);
        if(io != NULL)
            free(io);
        ++n;
    }
    snd_device_name_free_hint(hints);
}

QT_END_NAMESPACE
