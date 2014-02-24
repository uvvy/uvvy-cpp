#pragma once

#include "voicebox/audio_service.h"
#include "HostState.h"
#include <QObject>

class CallService : public QObject
{
    Q_OBJECT

    voicebox::audio_service audioclient_;

public:
    CallService(HostState& s, QObject *parent = nullptr);

    bool isCallActive() const;

public slots:
    void makeCall(QString callee_eid);
    void hangUp();

signals:
    void callStarted();
    void callFinished();
};
