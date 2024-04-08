#ifndef SIGNALMONITOREXTENSION_H
#define SIGNALMONITOREXTENSION_H


#include <QCoreApplication>
#include <QThread>
#include "octproz_devkit.h"
#include "signalmonitorform.h"
#include "imagemetriccalculator.h"

#define NUMBER_OF_BUFFERS 2


class SignalMonitor : public Extension
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID Extension_iid)
	Q_INTERFACES(Extension)
	QThread metricCalculatorThread;

public:
	SignalMonitor();
	~SignalMonitor();

	virtual QWidget* getWidget() override;
	virtual void activateExtension() override;
	virtual void deactivateExtension() override;
	virtual void settingsLoaded(QVariantMap settings) override;

private:
	SignalMonitorForm* form;
	ImageMetricCalculator* metricCalculator;
	BUFFER_SOURCE bufferSource;
	int frameNr;
	int bufferNr;
	int nthBuffer;
	int bufferCounter;
	bool active;
	bool isCalculating;

	QVector<void*> frameBuffersRaw;
	QVector<void*> frameBuffersProcessed;
	int copyBufferId;
	size_t bytesPerFrameRaw;
	size_t bytesPerFrameProcessed;
	int lostBuffersRaw;
	int lostBuffersProcessed;
	unsigned int framesPerBuffer;
	unsigned int buffersPerVolume;

	void setupGuiConnections();
	void setupMetricCalculator();
	void initializeFrameBuffers();
	void releaseFrameBuffers(QVector<void*> buffers);

public slots:
	void storeParameters();
	virtual void rawDataReceived(void* buffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr) override;
	virtual void processedDataReceived(void* buffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr) override;

signals:
	void newFrame(void* frame, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame);
	void maxFrames(int max);
	void maxBuffers(int max);
};

#endif //SIGNALMONITOREXTENSION_H
