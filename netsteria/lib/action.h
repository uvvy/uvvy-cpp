#pragma once

#include <QObject>
#include <QString>
#include <QList>

class QTimer;


class Action : public QObject
{
    Q_OBJECT

public:
    enum Status {
        Fresh,          // Brand-new, not yet started
        Running,        // Apparently making good progress
        Success,        // Action successfully completed
        Stalled,        // Temporarily not making progress
        Paused,         // Paused at user's request
        Error,          // Blocked by potentially fixable error
        FatalError,     // Blocked by permanent fatal error
    };

    static const int defaultRetryDelay = 60*1000;       // 1 minute

private:
    const QString desc;
    Status st;
    bool pausereq;
    QString note;
    float progr;
    QList<Action*> parts;
    QTimer *retrytimer;

public:
    inline Status status() { return st; }
    inline bool isFresh() { return st == Fresh; }
    inline bool isRunning() { return st == Running; }
    inline bool isSuccess() { return st == Success; }
    inline bool isStalled() { return st == Stalled; }
    inline bool isPaused() { return st == Paused; }
    inline bool isError() { return st == Error; }
    inline bool isFatalError() { return st == FatalError; }
    inline bool isBlocked() { return st == Error || st == FatalError; }
    inline bool isDone() { return st == Success || st == FatalError; }

    inline QString description() { return desc; }
    inline QString statusString() { return note; }
    inline float progress() { return progr; }

    // Actions can be composed of multiple sub-actions,
    // which may run in series or parallel in any fashion.
    inline int numParts() { return parts.size(); }
    inline Action *part(int idx) { return parts.value(idx); }

    // Abstract methods that the subclass must implement
    virtual void start() = 0;   // Start or restart

    // Client-controllable pause request flag
    // XX eventually want to add client-controllable prioritization, etc.
    inline bool pauseRequested() { return pausereq; }
    virtual void setPauseRequested(bool pausereq);
    inline void pause() { setPauseRequested(true); }
    inline void resume() { setPauseRequested(false); }

signals:
    void statusChanged();
    void progressChanged(float percent);

    void beforeInsertParts(int first, int last);
    void afterInsertParts(int first, int last);
    void beforeRemoveParts(int first, int last);
    void afterRemoveParts(int first, int last);


protected:
    Action(QObject *parent, const QString &description);

    // Set the current status.  Must not change from a 'done' state.
    void setStatus(Status newstatus, const QString &statustext);

    inline void setProgress(float newratio)
        { progr = newratio; progressChanged(newratio); }

    // Set the current status to indicate a non-fatal error,
    // and start a one-shot timer to call start() automatically
    // after a specified or default retryy delay.
    void setError(const QString &errortext,
            int retryDelay = defaultRetryDelay);

    // Lower-level methods for controlling the retry timer explicitly.
    void startRetryTimer(int delay = defaultRetryDelay);
    void stopRetryTimer();

    // Manage the list of parts comprising this action.
    void insertPart(int idx, Action *act, bool signal = true);
    inline void appendPart(Action *act, bool signal = true)
        { return insertPart(numParts(), act, signal); }
    void removePart(int idx, bool signal = true);
    void deletePart(int idx);
    void deleteAllParts();


private slots:
    void retryTimeout();
};
