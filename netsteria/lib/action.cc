
#include <QTimer>

#include "action.h"


Action::Action(QObject *parent, const QString &description)
:	QObject(parent), desc(description),
	st(Fresh), progr(-1.0),
	retrytimer(NULL)
{
}

void Action::setStatus(Status newstatus, const QString &statustext)
{
	// Not allowed to change status after we're in a Done state.
	Q_ASSERT(!isDone());

	if (st == newstatus && note == statustext)
		return;		// Nothing actually changed

	st = newstatus;
	note = statustext;

	// Notify anyone interested
	statusChanged();
}

void Action::setPauseRequested(bool pausereq)
{
	this->pausereq = pausereq;
}

void Action::setError(const QString &errortext, int retryDelay)
{
	startRetryTimer(retryDelay);
	setStatus(Error, errortext);
}

void Action::startRetryTimer(int delay)
{
	if (!retrytimer) {
		retrytimer = new QTimer(this);
		retrytimer->setSingleShot(true);
		connect(retrytimer, SIGNAL(timeout()),
			this, SLOT(retryTimeout()));
	}

	// XX jitter the retry timer?
	retrytimer->start(delay);
}

void Action::stopRetryTimer()
{
	if (!retrytimer)
		return;

	retrytimer->stop();
}

void Action::retryTimeout()
{
	if (!isRunning())
		start();
}

void Action::insertPart(int idx, Action *act, bool signal)
{
	Q_ASSERT(idx >= 0 && idx <= numParts());
	Q_ASSERT(act);
	Q_ASSERT(!parts.contains(act));

	if (signal)
		beforeInsertParts(idx, idx);

	act->setParent(this);
	parts.insert(idx, act);

	if (signal)
		afterInsertParts(idx, idx);
}

void Action::removePart(int idx, bool signal)
{
	Q_ASSERT(idx >= 0 && idx < numParts());

	if (signal)
		beforeRemoveParts(idx, idx);

	parts.removeAt(idx);

	if (signal)
		afterRemoveParts(idx, idx);
}

void Action::deletePart(int idx)
{
	Q_ASSERT(idx >= 0 && idx < numParts());

	part(idx)->deleteLater();
	removePart(idx);
}

void Action::deleteAllParts()
{
	int nparts = numParts();
	if (nparts == 0)
		return;

	beforeRemoveParts(0, nparts-1);

	for (int i = 0; i < nparts; i++)
		part(i)->deleteLater();
	parts.clear();

	afterRemoveParts(0, nparts-1);
}

