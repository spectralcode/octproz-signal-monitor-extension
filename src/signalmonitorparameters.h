#ifndef SIGNALMONITORPARAMETERS_H
#define SIGNALMONITORPARAMETERS_H

#include <QString>
#include <QtGlobal>
#include <QMetaType>
#include <QRect>

#define SIGNALMONITOR_SAMPLES_IN_PLOT "visible_samples"
#define SIGNALMONITOR_SOURCE "image_source"
#define SIGNALMONITOR_METRIC "metric"
#define SIGNALMONITOR_FRAME "frame_number"
#define SIGNALMONITOR_BUFFER "buffer_number"
#define SIGNALMONITOR_NTHBUFFER "nth_buffer_to_use"
#define SIGNALMONITOR_ROI_X "roi_x"
#define SIGNALMONITOR_ROI_Y "roi_y"
#define SIGNALMONITOR_ROI_WIDTH "roi_width"
#define SIGNALMONITOR_ROI_HEIGHT "roi_height"
#define SIGNALMONITOR_WINDOW_STATE "window_state"

enum BUFFER_SOURCE{
	RAW,
	PROCESSED
};

enum IMAGE_METRIC{
	SUM,
	AVERAGE,
	STDDEV,
	COEFFVAR
};

struct SignalMonitorParameters {
	BUFFER_SOURCE bufferSource;
	IMAGE_METRIC imageMetric;
	QRect roi;
	int frameNr;
	int bufferNr;
	int nthBufferToUse;
	int visibleSamples;
	QByteArray windowState;
};
Q_DECLARE_METATYPE(SignalMonitorParameters)


#endif //SIGNALMONITORPARAMETERS_H
