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

#include <QtCore/qdebug.h>

#include <qaudiodeviceinfo.h>
#include "qaudioengine_gstreamer_p.h"
#include "qaudiodeviceinfo_gstreamer_p.h"
#include "qaudiooutput_gstreamer_p.h"
#include <sys/types.h>
#include <unistd.h>

#include <private/qgstutils_p.h>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QGStreamerAudioEngine, gstreamerEngine);

QGStreamerAudioEngine::QGStreamerAudioEngine(QObject *parent)
    : QObject(parent)
{
    updateDevices();
}

void QGStreamerAudioEngine::updateDevices()
{
    QGstUtils::initializeGst();

    const auto sources = QGstUtils::audioSources();

    auto deviceList = [](const auto &deviceSet) {
        QList<QByteArray> devices;
        for (auto *d : deviceSet) {
            auto *properties = gst_device_get_properties(d);
            if (properties) {
                auto *klass = gst_structure_get_string(properties, "device.class");
                if (strcmp(klass, "monitor")) {
                    auto *name = gst_structure_get_string(properties, "sysfs.path");
                    gboolean def;
                    if (gst_structure_get_boolean(properties, "is-default", &def) && def)
                        devices.prepend(name);
                    else
                        devices.append(name);
                }

                gst_structure_free(properties);
            }
        }
        return devices;
    };

    m_sources = deviceList(QGstUtils::audioSources());
    m_sinks = deviceList(QGstUtils::audioSinks());
}

QGStreamerAudioEngine *QGStreamerAudioEngine::instance()
{
    return gstreamerEngine();
}

QList<QByteArray> QGStreamerAudioEngine::availableDevices(QAudio::Mode mode) const
{
    return (mode == QAudio::AudioOutput) ? m_sinks : m_sources;
}

QByteArray QGStreamerAudioEngine::defaultDevice(QAudio::Mode mode) const
{
    const QList<QByteArray> &devices = availableDevices(mode);
    return devices.size() ? devices.at(0) : QByteArray();
}

QT_END_NAMESPACE