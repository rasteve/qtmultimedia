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

#ifndef CAMERABINFOCUSCONTROL_H
#define CAMERABINFOCUSCONTROL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qcamera.h>
#include <qcamerafocuscontrol.h>

#include <private/qgstreamerbufferprobe_p.h>

#include <qbasictimer.h>
#include <qlist.h>
#include <qmutex.h>

#include <gst/gst.h>
#include <glib.h>

QT_BEGIN_NAMESPACE

class CameraBinSession;

class CameraBinFocus
    : public QCameraFocusControl
    , QGstreamerBufferProbe
{
    Q_OBJECT

public:
    CameraBinFocus(CameraBinSession *session);
    virtual ~CameraBinFocus();

    QCameraFocus::FocusModes focusMode() const override;
    void setFocusMode(QCameraFocus::FocusModes mode) override;
    bool isFocusModeSupported(QCameraFocus::FocusModes mode) const override;

    QCameraFocus::FocusPointMode focusPointMode() const override;
    void setFocusPointMode(QCameraFocus::FocusPointMode mode) override;
    bool isFocusPointModeSupported(QCameraFocus::FocusPointMode) const override;
    QPointF customFocusPoint() const override;
    void setCustomFocusPoint(const QPointF &point) override;

    QCameraFocusZoneList focusZones() const override;


    qreal maximumOpticalZoom() const override;
    qreal maximumDigitalZoom() const override;

    qreal requestedOpticalZoom() const override;
    qreal requestedDigitalZoom() const override;
    qreal currentOpticalZoom() const override;
    qreal currentDigitalZoom() const override;

    void zoomTo(qreal optical, qreal digital) override;

    void setViewfinderResolution(const QSize &resolution);

protected:
    void timerEvent(QTimerEvent *event) override;

private Q_SLOTS:
    void _q_handleCameraStatusChange(QCamera::Status status);
    void _q_updateFaces();

private:
    void resetFocusPoint();
    void updateRegionOfInterest(const QRectF &rectangle);
    void updateRegionOfInterest(const QList<QRect> &rectangles);
    bool probeBuffer(GstBuffer *buffer) override;

    CameraBinSession *m_session;
    QCamera::Status m_cameraStatus;
    QCameraFocus::FocusModes m_focusMode;
    QCameraFocus::FocusPointMode m_focusPointMode;
    QCameraFocusZone::FocusZoneStatus m_focusZoneStatus;
    QPointF m_focusPoint;
    QRectF m_focusRect;
    QSize m_viewfinderResolution;
    QList<QRect> m_faces;
    QList<QRect> m_faceFocusRects;
    QBasicTimer m_faceResetTimer;
    mutable QMutex m_mutex;

    static void updateZoom(GObject *o, GParamSpec *p, gpointer d);
    static void updateMaxZoom(GObject *o, GParamSpec *p, gpointer d);

    qreal m_requestedOpticalZoom = 1.;
    qreal m_requestedDigitalZoom = 1.;

};

QT_END_NAMESPACE

#endif // CAMERABINFOCUSCONTROL_H
