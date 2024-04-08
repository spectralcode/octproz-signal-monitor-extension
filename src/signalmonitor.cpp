#include "signalmonitor.h"


SignalMonitor::SignalMonitor()
	: Extension(),
	form(new SignalMonitorForm()),
	metricCalculator(new ImageMetricCalculator()),
	bufferCounter(0),
	copyBufferId(-1),
	bytesPerFrameProcessed(0),
	bytesPerFrameRaw(0),
	isCalculating(false),
	active(false),
	bufferNr(0),
	nthBuffer(10),
	frameNr(0)
{
	qRegisterMetaType<SignalMonitorParameters>("SignalMonitorParameters");

	this->setType(EXTENSION);
	this->displayStyle = SEPARATE_WINDOW;
	this->name = "Signal Monitor";
	this->toolTip = "OCT signal strength monitor";

	this->setupGuiConnections();
	this->setupMetricCalculator();
	this->initializeFrameBuffers();
}

SignalMonitor::~SignalMonitor() {
	metricCalculatorThread.quit();
	metricCalculatorThread.wait();

	delete this->form;

	this->releaseFrameBuffers(this->frameBuffersProcessed);
	this->releaseFrameBuffers(this->frameBuffersRaw);
}

QWidget* SignalMonitor::getWidget() {
	return this->form;
}

void SignalMonitor::activateExtension() {
	//this method is called by OCTproZ as soon as user activates the extension. If the extension controls hardware components, they can be prepared, activated, initialized or started here.
	this->active = true;
}

void SignalMonitor::deactivateExtension() {
	//this method is called by OCTproZ as soon as user deactivates the extension. If the extension controls hardware components, they can be deactivated, resetted or stopped here.
	this->active = false;
}

void SignalMonitor::settingsLoaded(QVariantMap settings) {
	//this method is called by OCTproZ and provides a QVariantMap with stored settings/parameters.
	this->form->setSettings(settings); //update gui with stored settings
}

void SignalMonitor::setupGuiConnections() {
	connect(this->form, &SignalMonitorForm::info, this, &SignalMonitor::info);
	connect(this->form, &SignalMonitorForm::error, this, &SignalMonitor::error);
	connect(this, &SignalMonitor::maxBuffers, this->form, &SignalMonitorForm::setMaximumBufferNr);
	connect(this, &SignalMonitor::maxFrames, this->form, &SignalMonitorForm::setMaximumFrameNr);

	//store settings
	connect(this->form, &SignalMonitorForm::paramsChanged, this, &SignalMonitor::storeParameters);

	//image display connections
	ImageDisplay* imageDisplay = this->form->getImageDisplay();
	connect(this, &SignalMonitor::newFrame, imageDisplay, &ImageDisplay::receiveFrame);
	connect(imageDisplay, &ImageDisplay::roiChanged, this, [this](const QRect& rect) {
		QString rectString = QString("ROI: %1, %2, %3, %4").arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height());
		emit this->info(rectString);
	});

	//data acquisition settings inputs from the GUI
	connect(this->form, &SignalMonitorForm::nthBufferChanged, this, [this](int nthBuffer) {
		this->nthBuffer = nthBuffer;
	});
	connect(this->form, &SignalMonitorForm::frameNrChanged, this, [this](int frameNr) {
		this->frameNr = frameNr;
	});
	connect(this->form, &SignalMonitorForm::bufferNrChanged, this, [this](int bufferNr) {
		this->bufferNr = bufferNr;
	});
	connect(this->form, &SignalMonitorForm::bufferSourceChanged, this, [this](BUFFER_SOURCE source) {
		this->bufferSource = source;
	});
}

void SignalMonitor::setupMetricCalculator() {
	this->metricCalculator = new ImageMetricCalculator();
	this->metricCalculator->moveToThread(&metricCalculatorThread);
	ImageDisplay* imageDisplay = this->form->getImageDisplay();
	connect(this, &SignalMonitor::newFrame, this->metricCalculator, &ImageMetricCalculator::calculateMetric);
	connect(imageDisplay, &ImageDisplay::roiChanged, this->metricCalculator, &ImageMetricCalculator::setRoi);
	connect(this->metricCalculator, &ImageMetricCalculator::metricCalculated, this->form, &SignalMonitorForm::displayCurrentMetricValue);
	connect(this->form, &SignalMonitorForm::imageMetricChanged, this->metricCalculator, &ImageMetricCalculator::setMetric);
	connect(this->metricCalculator, &ImageMetricCalculator::info, this, &SignalMonitor::info);
	connect(this->metricCalculator, &ImageMetricCalculator::error, this, &SignalMonitor::error);
	connect(&metricCalculatorThread, &QThread::finished, this->metricCalculator, &QObject::deleteLater);
	metricCalculatorThread.start();
}

void SignalMonitor::initializeFrameBuffers() {
	this->frameBuffersRaw.resize(NUMBER_OF_BUFFERS);
	this->frameBuffersProcessed.resize(NUMBER_OF_BUFFERS);
	for(int i = 0; i < NUMBER_OF_BUFFERS; i++){
		this->frameBuffersRaw[i] = nullptr;
		this->frameBuffersProcessed[i] = nullptr;
	}
}

void SignalMonitor::releaseFrameBuffers(QVector<void *> buffers) {
	for (int i = 0; i < buffers.size(); i++) {
		if (buffers[i] != nullptr) {
			free(buffers[i]);
			buffers[i] = nullptr;
		}
	}
}

void SignalMonitor::storeParameters() {
	//update settingsMap, so parameters can be reloaded into gui at next start of application
	this->form->getSettings(&this->settingsMap);
	emit storeSettings(this->name, this->settingsMap);
}

void SignalMonitor::rawDataReceived(void* buffer, unsigned bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr) {
	if(this->bufferSource == RAW && this->active){
		if(!this->isCalculating && this->rawGrabbingAllowed){

			//check if this is the nthBuffer
			this->bufferCounter++;
			if(this->bufferCounter < this->nthBuffer){
				return;
			}
			this->bufferCounter = 0;

			this->isCalculating = true;

			//calculate size of single frame
			size_t bytesPerSample = static_cast<size_t>(ceil(static_cast<double>(bitDepth)/8.0));
			size_t bytesPerFrame = samplesPerLine*linesPerFrame*bytesPerSample;

			//check if number of frames per buffer has changed and emit maxFrames to update gui
			if(this->framesPerBuffer != framesPerBuffer){
				emit maxFrames(framesPerBuffer-1);
				this->framesPerBuffer = framesPerBuffer;
			}
			//check if number of buffers per volume has changed and emit maxBuffers to update gui
			if(this->buffersPerVolume != buffersPerVolume){
				emit maxBuffers(buffersPerVolume-1);
				this->buffersPerVolume = buffersPerVolume;
			}

			//check if buffer size changed and allocate buffer memory
			if(this->frameBuffersRaw[0] == nullptr || this->bytesPerFrameRaw != bytesPerFrame){
				if(bitDepth == 0 || samplesPerLine == 0 || linesPerFrame == 0 || framesPerBuffer == 0){
					emit error(this->name + ":  " + tr("Invalid data dimensions!"));
					return;
				}
				//(re)create copy buffers
				if(this->frameBuffersRaw[0] != nullptr){
					this->releaseFrameBuffers(this->frameBuffersRaw);
				}
				for (int i = 0; i < this->frameBuffersRaw.size(); i++) {
					this->frameBuffersRaw[i] = static_cast<void*>(malloc(bytesPerFrame));
				}
				this->bytesPerFrameRaw = bytesPerFrame;
			}

			//copy single frame of received data and emit it for further processing
			this->copyBufferId = (this->copyBufferId+1)%NUMBER_OF_BUFFERS;
			char* frameInBuffer = static_cast<char*>(buffer);
			if(this->frameNr>static_cast<int>(framesPerBuffer-1)){this->frameNr = static_cast<int>(framesPerBuffer-1);}
			if(this->bufferNr>static_cast<int>(buffersPerVolume-1)){this->bufferNr = static_cast<int>(buffersPerVolume-1);}
			if(this->bufferNr == -1 || this->bufferNr == static_cast<int>(currentBufferNr)){
				memcpy(this->frameBuffersRaw[this->copyBufferId], &(frameInBuffer[bytesPerFrame*this->frameNr]), bytesPerFrame);
				emit newFrame(this->frameBuffersRaw[this->copyBufferId], bitDepth, samplesPerLine, linesPerFrame);
			}

			this->isCalculating = false;
		}
		else{
			this->lostBuffersRaw++;
			emit info(this->name + ": " + tr("Raw buffer lost. Total lost raw buffers: ") + QString::number(lostBuffersRaw));
			if(this->lostBuffersRaw>= INT_MAX){
				this->lostBuffersRaw = 0;
				emit info(this->name + ": " + tr("Lost raw buffer counter overflow. Counter set to zero."));
			}
		}
	}
}

void SignalMonitor::processedDataReceived(void* buffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr) {
	if(this->bufferSource == PROCESSED && this->active){
		if(!this->isCalculating && this->processedGrabbingAllowed){
			//check if current buffer is selected. If it is not selected discard it and do nothing (just return).
			if(this->bufferNr>static_cast<int>(buffersPerVolume-1)){this->bufferNr = static_cast<int>(buffersPerVolume-1);}
			if(!(this->bufferNr == -1 || this->bufferNr == static_cast<int>(currentBufferNr))){
				return;
			}

			//check if this is the nthBuffer
			this->bufferCounter++;
			if(this->bufferCounter < this->nthBuffer){
				return;
			}
			this->bufferCounter = 0;

			this->isCalculating = true;

			//calculate size of single frame
			size_t bytesPerSample = static_cast<size_t>(ceil(static_cast<double>(bitDepth)/8.0));
			size_t bytesPerFrame = samplesPerLine*linesPerFrame*bytesPerSample;

			//check if number of frames per buffer has changed and emit maxFrames to update gui
			if(this->framesPerBuffer != framesPerBuffer){
				emit maxFrames(framesPerBuffer-1);
				this->framesPerBuffer = framesPerBuffer;
			}
			//check if number of buffers per volume has changed and emit maxBuffers to update gui
			if(this->buffersPerVolume != buffersPerVolume){
				emit maxBuffers(buffersPerVolume-1);
				this->buffersPerVolume = buffersPerVolume;
			}

			//check if buffer size changed and allocate buffer memory
			if(this->frameBuffersProcessed[0] == nullptr || this->bytesPerFrameProcessed != bytesPerFrame){
				if(bitDepth == 0 || samplesPerLine == 0 || linesPerFrame == 0 || framesPerBuffer == 0){
					emit error(this->name + ":  " + tr("Invalid data dimensions!"));
					return;
				}
				//(re)create copy buffers
				if(this->frameBuffersProcessed[0] != nullptr){
					this->releaseFrameBuffers(this->frameBuffersProcessed);
				}
				for (int i = 0; i < this->frameBuffersProcessed.size(); i++) {
					this->frameBuffersProcessed[i] = static_cast<void*>(malloc(bytesPerFrame));
				}
				this->bytesPerFrameProcessed = bytesPerFrame;
			}

			//copy single frame of received data and emit it for further processing
			this->copyBufferId = (this->copyBufferId+1)%NUMBER_OF_BUFFERS;
			char* frameInBuffer = static_cast<char*>(buffer);
			if(this->frameNr>static_cast<int>(framesPerBuffer-1)){this->frameNr = static_cast<int>(framesPerBuffer-1);}
			memcpy(this->frameBuffersProcessed[this->copyBufferId], &(frameInBuffer[bytesPerFrame*this->frameNr]), bytesPerFrame);
			emit newFrame(this->frameBuffersProcessed[this->copyBufferId], bitDepth, samplesPerLine, linesPerFrame);

			this->isCalculating = false;
		}
		else{
			this->lostBuffersProcessed++;
			emit info(this->name + ": " + tr("Processed buffer lost. Total lost buffers: ") + QString::number(lostBuffersProcessed));
			if(this->lostBuffersProcessed>= INT_MAX){
				this->lostBuffersProcessed = 0;
				emit info(this->name + ": " + tr("Lost processed buffer counter overflow. Counter set to zero."));
			}
		}
	}
}
