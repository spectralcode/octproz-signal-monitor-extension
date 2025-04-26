#include "signalmonitorform.h"
#include "ui_signalmonitorform.h"
#include <QPropertyAnimation>

SignalMonitorForm::SignalMonitorForm(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SignalMonitorForm) {
	ui->setupUi(this);
	
	this->scrollingPlot = this->ui->widget_scrollingPlot;
	this->scrollingPlot->setCurveColor(QColor(55, 100, 250));
	connect(this->scrollingPlot, &ScrollingPlot::info, this, &SignalMonitorForm::info);
	connect(this->scrollingPlot, &ScrollingPlot::error, this, &SignalMonitorForm::error);
	
	this->imageDisplay = this->ui->widget_imageDisplay;
	connect(this->imageDisplay, &ImageDisplay::info, this, &SignalMonitorForm::info);
	connect(this->imageDisplay, &ImageDisplay::error, this, &SignalMonitorForm::error);
	connect(this->imageDisplay, QOverload<QRect>::of(&ImageDisplay::roiChanged),this, [this](QRect roiRect) {
		this->parameters.roi = roiRect;
		emit roiChanged(roiRect);
		emit paramsChanged();
	});
	
	//init settings area
	connect(ui->toolButton_settings, &QToolButton::clicked, this, &SignalMonitorForm::toggleSettingsArea);
	this->ui->widget_settings->setVisible(false);
	this->adjustSize();

	//number of visible samples
	connect(this->ui->spinBox_visibleSamplesInPlot, QOverload<int>::of(&QSpinBox::valueChanged),this, [this](int visibleSamples) {
		this->parameters.visibleSamples = visibleSamples;
		this->scrollingPlot->setNumberOfVisibleDataPoints(visibleSamples);
		emit paramsChanged();
	});
		
	//ComboBox Image Metric
	QStringList metricOptions = {"Sum", "Average", "Standard deviation", "Coeff. of Variation"};
	this->ui->comboBox_imageMetric->addItems(metricOptions);
	connect(this->ui->comboBox_imageMetric, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
		this->parameters.imageMetric = static_cast<IMAGE_METRIC>(index); 
		emit imageMetricChanged(index);
		emit paramsChanged();
		this->scrollingPlot->clearPlot();
	});
	
	//ComboBox Image input
	QStringList srcOptions = { "Raw", "Processed"};
	this->ui->comboBox_imageSource->addItems(srcOptions);
	connect(this->ui->comboBox_imageSource, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
		this->parameters.bufferSource = static_cast<BUFFER_SOURCE>(index);
		emit bufferSourceChanged(this->parameters.bufferSource);
		emit paramsChanged();
	});

	//SpinBox nthBuffer
	connect(this->ui->spinBox_nthBuffer, QOverload<int>::of(&QSpinBox::valueChanged), [this](int nthBuffer) {
		this->parameters.nthBufferToUse = nthBuffer;
		emit nthBufferChanged(nthBuffer); 
		emit paramsChanged();
	});

	//SpinBox Buffer
	this->ui->spinBox_buffer->setMaximum(2);
	this->ui->spinBox_buffer->setMinimum(-1);
	this->ui->spinBox_buffer->setSpecialValueText(tr("All"));
	connect(this->ui->spinBox_buffer, QOverload<int>::of(&QSpinBox::valueChanged),[this](int bufferNr) {
		this->parameters.bufferNr = bufferNr;
		emit bufferNrChanged(bufferNr);
		emit paramsChanged();
	});

	//Frame slider and spinBox
	connect(this->ui->horizontalSlider_frame, &QSlider::valueChanged, this->ui->spinBox_frame, &QSpinBox::setValue);
	connect(this->ui->spinBox_frame, QOverload<int>::of(&QSpinBox::valueChanged), this->ui->horizontalSlider_frame, &QSlider::setValue);
	connect(this->ui->horizontalSlider_frame, &QSlider::valueChanged, this, [this](int frameNr) {
		this->parameters.frameNr = frameNr;
		emit frameNrChanged(frameNr);
		emit paramsChanged();
	});
	this->setMaximumFrameNr(512);

	//window size and position changes are intercepted by event filter
	this->installEventFilter(this);

	//default values
	this->parameters.bufferNr= -1;
	this->parameters.bufferSource = PROCESSED;
	this->parameters.frameNr = 0;
	this->parameters.imageMetric = AVERAGE;
	this->parameters.nthBufferToUse = 10;
	this->parameters.roi = QRect(50,50, 400, 800);
	this->parameters.visibleSamples = 256;
}

SignalMonitorForm::~SignalMonitorForm() {
	delete ui;
}

void SignalMonitorForm::setSettings(QVariantMap settings) {
	//update parameters struct
	if(!settings.isEmpty()){
		this->parameters.bufferNr = settings.value(SIGNALMONITOR_BUFFER).toInt();
		this->parameters.bufferSource = static_cast<BUFFER_SOURCE>(settings.value(SIGNALMONITOR_SOURCE).toInt());
		this->parameters.imageMetric = static_cast<IMAGE_METRIC>(settings.value(SIGNALMONITOR_METRIC).toInt());
		this->parameters.frameNr = settings.value(SIGNALMONITOR_FRAME).toInt();
		this->parameters.nthBufferToUse = settings.value(SIGNALMONITOR_NTHBUFFER).toInt();
		int roiX = settings.value(SIGNALMONITOR_ROI_X).toInt();
		int roiY = settings.value(SIGNALMONITOR_ROI_Y).toInt();
		int roiWidth = settings.value(SIGNALMONITOR_ROI_WIDTH).toInt();
		int roiHeight = settings.value(SIGNALMONITOR_ROI_HEIGHT).toInt();
		this->parameters.roi = QRect(roiX, roiY, roiWidth, roiHeight);
		this->parameters.windowState = settings.value(SIGNALMONITOR_WINDOW_STATE).toByteArray();
	}

	//update gui elements
	this->ui->spinBox_visibleSamplesInPlot->setValue(this->parameters.visibleSamples);
	this->ui->spinBox_buffer->setValue(this->parameters.bufferNr);
	this->ui->comboBox_imageSource->setCurrentIndex(static_cast<int>(this->parameters.bufferSource));
	this->ui->comboBox_imageMetric->setCurrentIndex(static_cast<int>(this->parameters.imageMetric));
	this->ui->horizontalSlider_frame->setValue(this->parameters.frameNr);
	this->ui->spinBox_nthBuffer->setValue(this->parameters.nthBufferToUse);
	this->ui->widget_imageDisplay->setRoi(this->parameters.roi);
	this->restoreGeometry(this->parameters.windowState);
}

void SignalMonitorForm::getSettings(QVariantMap* settings) {
	if (!settings){
		return;
	}

	settings->insert(SIGNALMONITOR_BUFFER, this->parameters.bufferNr);
	settings->insert(SIGNALMONITOR_SOURCE, static_cast<int>(this->parameters.bufferSource));
	settings->insert(SIGNALMONITOR_METRIC, static_cast<int>(this->parameters.imageMetric));
	settings->insert(SIGNALMONITOR_FRAME, this->parameters.frameNr);
	settings->insert(SIGNALMONITOR_NTHBUFFER, this->parameters.nthBufferToUse);
	settings->insert(SIGNALMONITOR_ROI_X, this->parameters.roi.x());
	settings->insert(SIGNALMONITOR_ROI_Y, this->parameters.roi.y());
	settings->insert(SIGNALMONITOR_ROI_WIDTH, this->parameters.roi.width());
	settings->insert(SIGNALMONITOR_ROI_HEIGHT, this->parameters.roi.height());
	settings->insert(SIGNALMONITOR_WINDOW_STATE, this->parameters.windowState);
}

bool SignalMonitorForm::eventFilter(QObject* watched, QEvent* event) {
	if (watched == this) {
		if (event->type() == QEvent::Resize || event->type() == QEvent::Move) {
			if (this->isVisible()) {
				this->parameters.windowState = this->saveGeometry();
				emit paramsChanged();
			}
		}
	}
	return QWidget::eventFilter(watched, event);
}


void SignalMonitorForm::toggleSettingsArea() {
	QWidget* settingsArea = this->ui->widget_settings;
	const int animationDuration = 300; //in milliseconds
	const int deltaHeight = settingsArea->minimumHeight();
	const int minHeightWhenHidden = 220;
	const int minHeightWhenShown = minHeightWhenHidden + 330;

	//prepare window height change animation
	QPropertyAnimation* windowHeightAnimation = new QPropertyAnimation(this, "geometry");
	windowHeightAnimation->setDuration(animationDuration); //animation duration in milliseconds
	QRect startGeometry = this->geometry();

	//prepare the settings widget height animation
	QPropertyAnimation* settingsHeightAnimation = new QPropertyAnimation(settingsArea, "maximumHeight");
	settingsHeightAnimation->setDuration(animationDuration); // Animation duration in milliseconds

	bool isSettingsAreaVisible = settingsArea->isVisible();
	if (isSettingsAreaVisible) {
		//if the widget is visible, prepare to hide the settings widget
		this->setMinimumHeight(minHeightWhenHidden);
		settingsHeightAnimation->setStartValue(deltaHeight);
		settingsHeightAnimation->setEndValue(0);
		settingsArea->setMinimumHeight(0);
		connect(settingsHeightAnimation, &QPropertyAnimation::finished, settingsArea, &QWidget::hide);
		connect(settingsHeightAnimation, &QPropertyAnimation::finished, this, [this, minHeightWhenHidden, settingsArea, deltaHeight]() {
			this->setMinimumHeight(minHeightWhenHidden);
			settingsArea->setMinimumHeight(deltaHeight);
		});
	} else {
		//show the settings widget
		settingsArea->show();
		settingsArea->setMaximumHeight(0);
		settingsHeightAnimation->setStartValue(0);
		settingsHeightAnimation->setEndValue(deltaHeight);
		settingsArea->setMinimumHeight(0);
		connect(settingsHeightAnimation, &QPropertyAnimation::finished, this, [this, minHeightWhenShown, settingsArea, deltaHeight]() {
			this->setMinimumHeight(minHeightWhenShown);
			settingsArea->setMinimumHeight(deltaHeight);
		});
	}

	//adjust the window target height based on the settings widget's visibility
	int windowTargetHeight = isSettingsAreaVisible ? this->height() - deltaHeight : this->height() + deltaHeight;
	QRect endGeometry = QRect(startGeometry.x(), startGeometry.y(), startGeometry.width(), windowTargetHeight);
	windowHeightAnimation->setStartValue(startGeometry);
	windowHeightAnimation->setEndValue(endGeometry);

	//start both animations
	windowHeightAnimation->start(QPropertyAnimation::DeleteWhenStopped);
	settingsHeightAnimation->start(QPropertyAnimation::DeleteWhenStopped);
}

void SignalMonitorForm::setMaximumFrameNr(int maximum) {
	this->ui->horizontalSlider_frame->setMaximum(maximum);
	this->ui->spinBox_frame->setMaximum(maximum);
}

void SignalMonitorForm::setMaximumBufferNr(int maximum) {
	this->ui->spinBox_buffer->setMaximum(maximum);
}

void SignalMonitorForm::displayCurrentMetricValue(qreal value) {
	this->ui->textEdit_currentValue->setText(QString::number(value));
	this->getScrollingPlot()->addDataToCurve(value);
}
