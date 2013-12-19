QString Audio::indevname;
QString Audio::outdevname;

// Last error string returned
QString Audio::errmsg;

int Audio::scan()
{
	if (indevname.isEmpty() || (indev = findDevice(indevname)) < 0 || inChannels(indev) <= 0)
		indev = defaultInputDevice();
	if (outdevname.isEmpty() || (outdev = findDevice(outdevname)) < 0 || outChannels(outdev) <= 0)
		outdev = defaultOutputDevice();

	int n = 0;
	foreach(QString s, deviceNames())
	{
		qDebug() << "Device" << n << ":" << s;
		++n;
	}
}

QString Audio::deviceName(int dev)
{
	RtAudio::DeviceInfo info;
    try {
      info = audio_inst->getDeviceInfo(dev);
    }
    catch (RtError &error) {
      qWarning("Error getting device %d name: %s", dev, error.what());
      return QString();
    }
	return QString::fromLocal8Bit(info.name.c_str());
}

QStringList Audio::deviceNames()
{
	QStringList names;
	for (int i = 0; i < ndevs; i++)
		names.append(deviceName(i));
	return names;
}

int Audio::findDevice(const QString &name)
{
	for (int i = 0; i < ndevs; i++)
		if (deviceName(i) == name)
			return i;
	return -1;
}

void Audio::setInputDevice(int dev)
{
	Q_ASSERT(dev >= 0 && dev < ndevs && inChannels(dev) > 0);
	close();
	indev = dev;
	open();
}

void Audio::setOutputDevice(int dev)
{
	Q_ASSERT(dev >= 0 && dev < ndevs && outChannels(dev) > 0);
	close();
	outdev = dev;
	open();
}

void Audio::setInputDevice(const QString &name)
{
	for (int i = 0; i < Audio::numDevices(); i++) {
		if (deviceName(i) == name && inChannels(i) > 0) {
			setInputDevice(i);
			return;
		}
	}
	qWarning() << "Audio: unknown input device" << name;
}

void Audio::setOutputDevice(const QString &name)
{
	for (int i = 0; i < Audio::numDevices(); i++) {
		if (deviceName(i) == name && outChannels(i) > 0) {
			setOutputDevice(i);
			return;
		}
	}
	qWarning() << "Audio: unknown output device" << name;
}

int Audio::inChannels(int dev)
{
	RtAudio::DeviceInfo info;
    try {
      info = audio_inst->getDeviceInfo(dev);
    }
    catch (RtError &error) {
      qWarning("Error getting device %d in channels: %s", dev, error.what());
      return 0;
    }
	return info.inputChannels;
}

int Audio::outChannels(int dev)
{
	RtAudio::DeviceInfo info;
    try {
      info = audio_inst->getDeviceInfo(dev);
    }
    catch (RtError &error) {
      qWarning("Error getting device %d out channels: %s", dev, error.what());
      return 0;
    }
	return info.outputChannels;
}

double Audio::minSampleRate(int dev)
{
	RtAudio::DeviceInfo info;
    try {
      info = audio_inst->getDeviceInfo(dev);
    }
    catch (RtError &error) {
      qWarning("Error getting device %d sample rates: %s", dev, error.what());
      return 0.0;
    }
	Q_ASSERT(info.sampleRates.size() != 0);
	int minrate = INT_MAX;
	foreach (int rate, info.sampleRates)
		minrate = qMin(minrate, rate);
	return minrate;
}

double Audio::maxSampleRate(int dev)
{
	RtAudio::DeviceInfo info;
    try {
      info = audio_inst->getDeviceInfo(dev);
    }
    catch (RtError &error) {
      qWarning("Error getting device %d sample rates: %s", dev, error.what());
      return 0.0;
    }
	Q_ASSERT(info.sampleRates.size() != 0);
	int maxrate = 0;
	foreach (int rate, info.sampleRates)
		maxrate = qMax(maxrate, rate);
	return maxrate;
}

int Audio::rescan()
{
	close();
	if (ndevs >= 0) {
		delete audio_inst; audio_inst = 0;
		ndevs = -1;
	}

	int rc = scan();
	open();

	return rc;
}

void Audio::setInputLevel(int lev)
{
	if (inlev == lev)
		return;

	qDebug() << "setInputLevel" << lev;
	inlev = lev;
	instance()->inputLevelChanged(lev);
}

void Audio::setOutputLevel(int lev)
{
	if (outlev == lev)
		return;

	qDebug() << "setOutputLevel" << lev;
	outlev = lev;
	instance()->outputLevelChanged(lev);
}

Audio::~Audio()
{
	close();
	if (ndevs >= 0)
	{
		delete audio_inst; audio_inst = 0;
	}
}
