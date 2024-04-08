#ifndef SIGNALMONITORFORM_H
#define SIGNALMONITORFORM_H

#include <QWidget>
#include <QRect>
#include "signalmonitorparameters.h"
#include "scrollingplot.h"
#include "imagedisplay.h"

namespace Ui {
class SignalMonitorForm;
}

class SignalMonitorForm : public QWidget
{
	Q_OBJECT

public:
	explicit SignalMonitorForm(QWidget *parent = 0);
	~SignalMonitorForm();

	void setSettings(QVariantMap settings);
	void getSettings(QVariantMap* settings);

	ScrollingPlot* getScrollingPlot(){return this->scrollingPlot;}
	ImageDisplay* getImageDisplay(){return this->imageDisplay;}

	Ui::SignalMonitorForm* ui;

protected:


public slots:
	void toggleSettingsArea();
	void setMaximumFrameNr(int maximum);
	void setMaximumBufferNr(int maximum);
	void displayCurrentMetricValue(qreal value);

private:
	ScrollingPlot* scrollingPlot;
	ImageDisplay* imageDisplay;
	QSize lastSize;
	SignalMonitorParameters parameters;

signals:
	void paramsChanged();
	void frameNrChanged(int);
	void bufferNrChanged(int);
	void imageMetricChanged(int);
	void nthBufferChanged(int);
	void bufferSourceChanged(BUFFER_SOURCE);
	void roiChanged(QRect);
	void info(QString);
	void error(QString);

};

#endif //SIGNALMONITORFORM_H
