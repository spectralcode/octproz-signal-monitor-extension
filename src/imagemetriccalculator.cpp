#include "imagemetriccalculator.h"


ImageMetricCalculator::ImageMetricCalculator(QObject *parent)
	: QObject(parent),
	calculationRunning(false),
	selectedMetric(IMAGE_METRIC::SUM)
{
	this->stats = {0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, 0, 1024, 1024};
	this->roi.setRect(0, 0, 1024, 1024);
}

QPoint ImageMetricCalculator::indexToPoint(int index, int width) {
	int x = index%width;
	int y = index/width;
	return QPoint(x, y);
}

qreal ImageMetricCalculator::standardDeviation(QVector<qreal> samples, qreal mean) {
	qreal sum = 0;
	for (int i = 0; i < samples.length(); i++){
		sum = sum + qPow(samples.at(i) - mean, 2);
	}
	return qSqrt(sum/(samples.length()));
}

void ImageMetricCalculator::calculateMetric(void* frameBuffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame) {
	if(!this->calculationRunning){
		this->calculationRunning = true;

		//set buffer datatype according bitdepth and start statistics calculation
		//uchar
		if(bitDepth <= 8){
			unsigned char* frame = static_cast<unsigned char*>(frameBuffer);
			this->calculateStatistics(frame, bitDepth, samplesPerLine, linesPerFrame);
		}
		//ushort
		else if(bitDepth > 8 && bitDepth <= 16){
			unsigned short* frame = static_cast<unsigned short*>(frameBuffer);
			this->calculateStatistics(frame, bitDepth, samplesPerLine, linesPerFrame);
		}
		//unsigned long int
		else if(bitDepth > 16 && bitDepth <= 32){
			unsigned long int* frame = static_cast<unsigned long int*>(frameBuffer);
			this->calculateStatistics(frame, bitDepth, samplesPerLine, linesPerFrame);
		}

		this->calculationRunning = false;
	}
}

void ImageMetricCalculator::setRoi(QRect roi) {
	this->roi = roi;
}

void ImageMetricCalculator::setMetric(int metric) {
	this->selectedMetric =static_cast<IMAGE_METRIC>(metric);
}

template<typename T>
void ImageMetricCalculator::calculateStatistics(T frame, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame) {
	//init params for statistic calculation
	int numberOfPossibleValues = static_cast<int>(pow(2, bitDepth));
	qreal sum = 0;
	qreal length = samplesPerLine*linesPerFrame;
	qreal maxValue = 0;
	qreal minValue = 999999999;
	int pixels = 0;
	QVector<qreal> samples;

	//statistics calculation
	for(int i = 0; i < length; i++){
		if(this->roi.contains(this->indexToPoint(i, static_cast<int>(samplesPerLine)))){
			qreal currValue = frame[i];
			samples.append(currValue);
			if(maxValue < currValue){maxValue = currValue;}
			if(minValue > currValue){minValue = currValue;}
			pixels++;
			sum += currValue;

			if(currValue>(numberOfPossibleValues-1)){
				currValue = numberOfPossibleValues-1;
			}
			if(currValue<0){
				currValue = 0;
			}
		}
	}

	//update ImageStatistics struct
	this->stats.max = maxValue;
	this->stats.min = minValue;
	this->stats.pixels = pixels;
	this->stats.sum = sum;
	this->stats.average = this->stats.sum/pixels;
	this->stats.stdDeviation = this->standardDeviation(samples, this->stats.average);
	this->stats.coeffOfVariation = this->stats.stdDeviation/this->stats.average;
	this->stats.roiX = this->roi.x();
	this->stats.roiY = this->roi.y();
	this->stats.roiWidth = this->roi.width();
	this->stats.roiHeight = this->roi.height();

	qreal metricValue = 0;
	switch(this->selectedMetric){
		case SUM:metricValue = this->stats.sum; break;
		case AVERAGE:metricValue =this->stats.average; break;
		case STDDEV: metricValue = this->stats.stdDeviation; break;
		case COEFFVAR: metricValue = this->stats.coeffOfVariation; break;
		default: metricValue = this->stats.sum;
	}
	emit metricCalculated(metricValue);
}
