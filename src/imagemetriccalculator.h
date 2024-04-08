#ifndef IMAGESMETRICCALCULATOR_H
#define IMAGESMETRICCALCULATOR_H

#define NUMBER_OF_HISTOGRAM_BUFFERS 2

#include <QObject>
#include <QVector>
#include <QRect>
#include <QApplication>
#include <QtMath>
#include "signalmonitorparameters.h"

struct ImageStatistics {
	int pixels;
	qreal max;
	qreal min;
	qreal sum;
	qreal average;
	qreal stdDeviation;
	qreal coeffOfVariation;
	int roiX;
	int roiY;
	int roiWidth;
	int roiHeight;
};

class ImageMetricCalculator : public QObject
{
	Q_OBJECT
public:
	explicit ImageMetricCalculator(QObject *parent = nullptr);

private:
	bool calculationRunning;
	ImageStatistics stats;
	QRect roi;
	IMAGE_METRIC selectedMetric;

	QPoint indexToPoint(int index, int width);
	qreal standardDeviation(QVector<qreal> samples, qreal mean);
	template <typename T> void calculateStatistics(T frame, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame);


signals:
	void metricCalculated(qreal);
	void info(QString);
	void error(QString);

public slots:
	void calculateMetric(void* frameBuffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame);
	void setRoi(QRect roi);
	void setMetric(int metric);
};

#endif //IMAGESMETRICCALCULATOR_H
