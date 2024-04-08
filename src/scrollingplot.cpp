#include "scrollingplot.h"
#include <QPainterPathStroker>

ScrollingPlot::ScrollingPlot(QWidget *parent) : QCustomPlot(parent){
	//default colors
	this->referenceCurveAlpha = 255;
	this->setBackground( QColor(50, 50, 50));
	this->axisRect()->setBackground(QColor(25, 25, 25));
	this->curveColor.setRgb(250, 100, 55);
	this->referenceCurveColor.setRgb(55, 250, 100, referenceCurveAlpha);

	//default appearance of legend
	this->legend->setBrush(QBrush(QColor(0,0,0,100))); //semi transparent background
	this->legend->setTextColor(QColor(200,200,200,255));
	QFont legendFont = font();
	legendFont.setPointSize(8);
	this->legend->setFont(legendFont);
	this->legend->setSelectedFont(legendFont);
	this->legend->setBorderPen(QPen(QColor(0,0,0,0))); //set legend border invisible
	this->legend->setColumnSpacing(0);
	this->legend->setRowSpacing(0);
	this->axisRect()->insetLayout()->setAutoMargins(QCP::msNone);
	this->axisRect()->insetLayout()->setMargins(QMargins(0,0,0,0));

	this->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);

	//configure curve graph
	this->addGraph();
	this->setCurveColor(curveColor);

	//configure reference curve graph
	this->addGraph();
	this->setReferenceCurveColor(referenceCurveColor);

	//configure axis
	this->setAxisVisible(false);
	this->setAxisColor(QColor(64, 64, 64));

	//configure grid
	this->xAxis->grid()->setPen(QPen(QColor(64, 64, 64), 1, Qt::DotLine));
	this->yAxis->grid()->setPen(QPen(QColor(64, 64, 64), 1, Qt::DotLine));

	//user interactions: dragging and vertical zooming
	this->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
	this->axisRect()->setRangeZoom(Qt::Vertical);

	//maximize size of plot area
	this->axisRect()->setAutoMargins(QCP::msNone);
	this->axisRect()->setMargins(QMargins(0,0,0,0));

	//set number of data points within scrolling plot
	this->setMaxNumberOfDataPoints(1000000);
	this->setNumberOfVisibleDataPoints(512);

	//init scaling
	this->customRange = false;
	this->autoYaxisScaling = false;

	this->dataPointCounter = 0;
}

ScrollingPlot::~ScrollingPlot() {
}

void ScrollingPlot::setCurveColor(QColor color) {
	this->curveColor = color;
	QPen curvePen = QPen(color);
	curvePen.setWidth(1);
	this->graph(0)->setPen(curvePen);
}

void ScrollingPlot::setReferenceCurveColor(QColor color) {
	this->referenceCurveColor = color;
	QPen referenceCurvePen = QPen(color);
	referenceCurvePen.setWidth(1);
	this->graph(1)->setPen(referenceCurvePen);
}

void ScrollingPlot::setCurveName(QString name) {
	this->graph(0)->setName(name);
	this->replot();
}

void ScrollingPlot::setReferenceCurveName(QString name) {
	this->graph(1)->setName(name);
	this->replot();
}

void ScrollingPlot::setLegendVisible(bool visible) {
	this->legend->setVisible(visible);
	this->replot();
}

void ScrollingPlot::setAxisVisible(bool visible) {
	this->yAxis->setVisible(visible);
	this->xAxis->setVisible(visible);
}

void ScrollingPlot::addDataToCurves(double curveDataPoint, double referenceDataPoint){
	this->dataPointCounter++;
	if(this->dataPointCounter > this->maxDataPoints){
		this->clearPlot();
	}

	this->graph(0)->addData(dataPointCounter, curveDataPoint);
	this->graph(1)->addData(dataPointCounter, referenceDataPoint);

	//scale y axis
	this->rescaleAxes(true); //todo: disable auto rescaling if user zooms with mouse wheel and reactive it after double click
	//this->graph(1)->rescaleValueAxis();
	//this->graph(0)->rescaleValueAxis();

	//auto scroll plot:
	this->xAxis->setRange(dataPointCounter, this->visibleDataPoints, Qt::AlignRight);
	this->xAxis2->setRange(dataPointCounter, this->visibleDataPoints, Qt::AlignRight);

	this->zoomOutSlightly();
	this->replot();
}

void ScrollingPlot::addDataToCurve(double curveDataPoint){
	this->dataPointCounter++;
	if(this->dataPointCounter > this->maxDataPoints){
		this->clearPlot();
	}

	this->graph(0)->addData(dataPointCounter, curveDataPoint);

	//auto scroll plot in x direction
	this->xAxis->setRange(dataPointCounter, this->visibleDataPoints, Qt::AlignRight);

	//adjust the y-axis range
	bool foundRange;
	QCPRange range = this->graph(0)->getValueRange(foundRange);
	if(foundRange) {
		double rangeSpan = range.size();
		double padding = rangeSpan * 0.1; //adding 10% padding above and below the actual data range. this looks nicer and the user can cleary see the min and max values since they are not directly on the top or bottom edge
		this->yAxis->setRange(range.lower - padding, range.upper + padding);
	}

	this->replot();
}

void ScrollingPlot::clearPlot() {
	this->graph(0)->data()->clear();
	this->graph(1)->data()->clear();
	this->replot();
	this->dataPointCounter = 0;
}

void ScrollingPlot::setAxisColor(QColor color) {
	this->xAxis->setBasePen(QPen(color, 1));
	this->yAxis->setBasePen(QPen(color, 1));
	this->xAxis->setTickPen(QPen(color, 1));
	this->yAxis->setTickPen(QPen(color, 1));
	this->xAxis->setSubTickPen(QPen(color, 1));
	this->yAxis->setSubTickPen(QPen(color, 1));
	this->xAxis->setTickLabelColor(color);
	this->yAxis->setTickLabelColor(color);
	this->xAxis->setLabelColor(color);
	this->yAxis->setLabelColor(color);
}

void ScrollingPlot::zoomOutSlightly() {
	this->yAxis->scaleRange(1.1, this->yAxis->range().center());
	this->xAxis->scaleRange(1.1, this->xAxis->range().center());
}

void ScrollingPlot::contextMenuEvent(QContextMenuEvent *event) {
	QMenu menu(this);
	QAction savePlotAction(tr("Save Plot as..."), this);
	connect(&savePlotAction, &QAction::triggered, this, &ScrollingPlot::saveToDisk);
	menu.addAction(&savePlotAction);
	QAction clearPlotAction(tr("Clear plot"), this);
	connect(&clearPlotAction, &QAction::triggered, this, &ScrollingPlot::clearPlot);
	menu.addAction(&clearPlotAction);
	menu.exec(event->globalPos());
}

void ScrollingPlot::mouseMoveEvent(QMouseEvent *event) {
	if(!(event->buttons() & Qt::LeftButton)){
		double x = this->xAxis->pixelToCoord(event->pos().x());
		double y = this->yAxis->pixelToCoord(event->pos().y());
		this->setToolTip(QString("%1 , %2").arg(x).arg(y));
	}else{
		QCustomPlot::mouseMoveEvent(event);
	}
}

void ScrollingPlot::changeEvent(QEvent *event) {
	if(event->ActivationChange){
		if(!this->isEnabled()){
			this->curveColor.setAlpha(55);
			this->referenceCurveColor.setAlpha(25);
			this->setCurveColor(this->curveColor);
			this->setReferenceCurveColor(this->referenceCurveColor);
			this->replot();
		} else {
			this->curveColor.setAlpha(255);
			this->referenceCurveColor.setAlpha(this->referenceCurveAlpha);
			this->setCurveColor(this->curveColor);
			this->setReferenceCurveColor(this->referenceCurveColor);
			this->replot();
		}
	}
	QCustomPlot::changeEvent(event);
}

void ScrollingPlot::mouseDoubleClickEvent(QMouseEvent* event) {
	this->rescaleAxes();
	this->zoomOutSlightly();
	if(customRange){
		this->scaleYAxis(this->customRangeLower, this->customRangeUpper);
	}
	this->replot();
	this->autoYaxisScaling = true;
	event->accept();
}

void ScrollingPlot::setMaxNumberOfDataPoints(int maxDataPoints) {
	this->maxDataPoints = maxDataPoints;
}

void ScrollingPlot::setNumberOfVisibleDataPoints(int visibleDataPoints) {
	this->visibleDataPoints = visibleDataPoints;
}

void ScrollingPlot::saveToDisk() {
	QString filters("Image (*.png);;Vector graphic (*.pdf);;CSV (*.csv)");
	QString defaultFilter("CSV (*.csv)");
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Plot"), QDir::currentPath(), filters, &defaultFilter);
	if(fileName == ""){
		emit error(tr("Save plot to disk canceled."));
		return;
	}
	bool saved = false;
	if(defaultFilter == "Image (*.png)"){
		saved = this->savePng(fileName);
	}else if(defaultFilter == "Vector graphic (*.pdf)"){
		saved = this->savePdf(fileName);
	}else if(defaultFilter == "CSV (*.csv)"){
		this->saveAllCurvesToFile(fileName);
	}
	if(saved){
		emit info(tr("Plot saved to ") + fileName);
	}else{
		emit error(tr("Could not save plot to disk."));
	}
}

void ScrollingPlot::scaleYAxis(double min, double max) {
	this->customRange = true;
	this->customRangeLower = min;
	this->customRangeUpper = max;
	this->yAxis->setRange(min, max);
	this->yAxis2->setRange(min, max);
	this->zoomOutSlightly();
	this->replot();
}

bool ScrollingPlot::saveCurveDataToFile(QString fileName) {
	bool saved = false;
	QFile file(fileName);
	if (file.open(QFile::WriteOnly|QFile::Truncate)) {
		QTextStream stream(&file);
		stream << "Sample Number" << ";" << "Sample Value" << "\n";
		int numberOfDataPoints = this->graph(0)->data()->size();
		for(int i = 0; i < numberOfDataPoints; i++){
			stream << QString::number(this->graph(0)->data()->at(i)->key) << ";" << QString::number(this->graph(0)->data()->at(i)->value) << "\n";
		}
	file.close();
	saved = true;
	}
	return saved;
}

bool ScrollingPlot::saveAllCurvesToFile(QString fileName) {
	if(this->curve.size() != this->referenceCurve.size()){
		return this->saveCurveDataToFile(fileName);
	}else{
		bool saved = false;
		QFile file(fileName);
		if (file.open(QFile::WriteOnly|QFile::Truncate)) {
			QTextStream stream(&file);
			stream << "Sample Number" << ";" << this->graph(0)->name() << ";" << this->graph(1)->name() << "\n";
			int numberOfDataPoints = this->graph(0)->data()->size();
			for(int i = 0; i < numberOfDataPoints; i++){
				stream << QString::number(this->graph(0)->data()->at(i)->key) << ";" << QString::number(this->graph(0)->data()->at(i)->value) << ";" << QString::number(this->graph(1)->data()->at(i)->value) << "\n";
			}
		file.close();
		saved = true;
		}
		return saved;
	}
}
