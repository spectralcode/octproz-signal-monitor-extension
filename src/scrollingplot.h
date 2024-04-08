#ifndef SCROLLINGPLOT_H
#define SCROLLINGPLOT_H

#include "qcustomplot.h"

class ScrollingPlot : public QCustomPlot
{
	Q_OBJECT
public:
	explicit ScrollingPlot(QWidget *parent = nullptr);
	~ScrollingPlot();

	void setCurveColor(QColor color);
	void setReferenceCurveColor(QColor color);
	void setCurveName(QString name);
	void setReferenceCurveName(QString name);
	void setLegendVisible(bool visible);
	void setAxisVisible(bool visible);
	void addDataToCurves(double curveDataPoint, double referenceDataPoint);
	void addDataToCurve(double curveDataPoint);
	void clearPlot();


private:
	void setAxisColor(QColor color);
	void zoomOutSlightly();

	QVector<qreal> sampleNumbers;
	QVector<qreal> curve;
	QVector<qreal> referenceCurve;
	QColor curveColor;
	QColor referenceCurveColor;
	int referenceCurveAlpha;
	QCPItemStraightLine* lineA;
	QCPItemStraightLine* lineB;
	double customRangeLower;
	double customRangeUpper;
	bool customRange;
	int dataPointCounter;
	int maxDataPoints;
	int visibleDataPoints;
	bool autoYaxisScaling;

protected:
	void contextMenuEvent(QContextMenuEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void changeEvent(QEvent* event) override;

signals:
	void info(QString info);
	void error(QString error);


public slots:
	virtual void mouseDoubleClickEvent(QMouseEvent* event) override;
	void setMaxNumberOfDataPoints(int maxDataPoints);
	void setNumberOfVisibleDataPoints(int visibleDataPoints);
	void saveToDisk();
	void scaleYAxis(double min, double max);
	bool saveCurveDataToFile(QString fileName);
	bool saveAllCurvesToFile(QString fileName);
};


#endif // SCROLLINGPLOT_H
