/**
 * Small tool to generate charts using QtCharts
 *
 * version 0.1, february, 2015
 *
 * Copyright (C) 2015- Fredrik Kihlander
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Fredrik Kihlander
 */

#include <QtCore/QCommandLineParser>

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

#include <QtCharts/QChartView>

#include <QtCharts/QBarSeries>
#include <QtCharts/QHorizontalBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QLegend>
#include <QtCharts/QBarCategoryAxis>

#include <QtCharts/QScatterSeries>

// TODO:
// * add support for save png from button-press
// * add support for config.json for the data!
// * add support for config dimmensions (no hardcode!)
// * add abillity to filter on type and category (better names needed)

enum chart_type
{
	CHART_TYPE_BAR_VERTICAL,
	CHART_TYPE_BAR_HORIZONTAL,
	CHART_TYPE_SCATTER,

	CHART_TYPE_CNT
};

const char* CHART_TYPE_NAME[CHART_TYPE_CNT] = {
	"bar-vertical",
	"bar-horizontal",
	"scatter"
};

QT_CHARTS_USE_NAMESPACE

chart_type chartTypeFromString(const QString& str)
{
	for(int i = 0; i < CHART_TYPE_CNT; ++i)
		if(str == CHART_TYPE_NAME[i])
			return (chart_type)i;
	return CHART_TYPE_CNT;
}

struct WcChartBarData
{
	QStringList     types;      // TODO: better name!
	QStringList     categories; // TODO: better name!
	QList<QBarSet*> sets;       // TODO: better name!
};

static void readBarCSV( const QString& csv_data, WcChartBarData* out_data )
{
	QStringList lines = csv_data.split('\n');

	out_data->types = lines.takeFirst().split(',');
	out_data->types.pop_front();

	for( QString& type : out_data->types )
		out_data->sets.append(new QBarSet(type));

	while(!lines.empty())
	{
		QStringList items = lines.takeFirst().split(',');

		out_data->categories << items.takeFirst().trimmed();

		bool success;
		for( int i = 0; i < items.size(); ++i )
			out_data->sets[i]->append(items[i].toFloat(&success));

		// TODO: handle parse error here!
	}
}

static void expandYAxisRange( QChart* chart, float ratio )
{
	float min_y =  100000.0f;
	float max_y = -100000.0f;

	for( QAbstractSeries* as : chart->series() )
	{
		QBarSeries* s = qobject_cast<QBarSeries*>(as);
		for( const QBarSet* set : s->barSets() )
		{
			for( int i = 0; i < set->count(); ++i )
			{
				if( set->at(i) < min_y ) min_y = set->at(i);
				if( set->at(i) > max_y ) max_y = set->at(i);
			}
		}
	}

	float y_extra = (max_y - min_y) * ratio;
	chart->axisY()->setRange(0.0f, max_y + y_extra);
}

static void expandXAxisRange( QChart* chart, float ratio )
{
	float min_x =  100000.0f;
	float max_x = -100000.0f;

	for( QAbstractSeries* as : chart->series() )
	{
		QHorizontalBarSeries* s = qobject_cast<QHorizontalBarSeries*>(as);
		for( const QBarSet* set : s->barSets() )
		{
			for( int i = 0; i < set->count(); ++i )
			{
				if( set->at(i) < min_x ) min_x = set->at(i);
				if( set->at(i) > max_x ) max_x = set->at(i);
			}
		}
	}

	float x_extra = (max_x - min_x) * ratio;
	chart->axisX()->setRange(0.0f, max_x + x_extra);
}

static void expandXYAxisRange( QChart* chart, float ratio )
{
	float min_x =  100000.0f;
	float max_x = -100000.0f;
	float min_y =  100000.0f;
	float max_y = -100000.0f;

	for( QAbstractSeries* as : chart->series() )
	{
		QXYSeries* s = qobject_cast<QXYSeries*>(as);
		for( const QPointF& p : s->points() )
		{
			if( p.x() < min_x ) min_x = p.x();
			if( p.x() > max_x ) max_x = p.x();
			if( p.y() < min_y ) min_y = p.y();
			if( p.y() > max_y ) max_y = p.y();
		}
	}

	float x_extra = (max_x - min_x) * ratio;
	float y_extra = (max_y - min_y) * ratio;

	chart->axisX()->setRange(min_x - x_extra, max_x + x_extra);
	chart->axisY()->setRange(min_y - y_extra, max_y + y_extra);
}

static QChart* createBarChart( const QString& csv_data, chart_type type )
{
	WcChartBarData bar_data;
	readBarCSV(csv_data, &bar_data);

	QAbstractBarSeries* series = type == CHART_TYPE_BAR_HORIZONTAL 
											? (QAbstractBarSeries*)new QHorizontalBarSeries
											: (QAbstractBarSeries*)new QBarSeries;

	for(QBarSet* set : bar_data.sets)
		series->append(set);

	QChart* chart = new QChart();
	chart->addSeries(series);

	QBarCategoryAxis *axis = new QBarCategoryAxis();
	axis->append(bar_data.categories);
	chart->createDefaultAxes();
	chart->setAxisY(axis, series);

	if(type == CHART_TYPE_BAR_HORIZONTAL)
		expandXAxisRange(chart, 0.05f);
	else
		expandYAxisRange(chart, 0.05f);

	return chart;
}

// static QChart* createBarVerticalChart( const QString& csv_data )
// {
// 	WcChartBarData bar_data;
// 	readBarCSV(csv_data, &bar_data);
// 
// 	QBarSeries* series = new QBarSeries();
// 
// 	for(QBarSet* set : bar_data.sets)
// 		series->append(set);
// 
// 	QChart* chart = new QChart();
// 	chart->addSeries(series);
// 
// 	QBarCategoryAxis *axis = new QBarCategoryAxis();
// 	axis->append(bar_data.categories);
// 	chart->createDefaultAxes();
// 	chart->setAxisX(axis, series);
// 
// 	expandYAxisRange(chart, 0.05f);
// 
// 	return chart;
// }


static int findScatterSeriesCount( const QStringList& lines )
{
	for( int line = 1; line < lines.size(); ++line )
	{
		if(!lines[line].contains(","))
			return line - 1;
	}
	return -1;
}

static QChart* createScatterChart( const QString& csv_data )
{
	QStringList lines = csv_data.split('\n');
	int series_count = findScatterSeriesCount(lines);

	QList<QScatterSeries*> series;

	for( int i = 0; i < series_count; ++i )
		series.append(new QScatterSeries);

	while(!lines.empty())
	{
		QString title = lines.takeFirst();

		for( int i = 0; i < series_count; ++i )
		{
			QStringList items = lines.takeFirst().split(',');
			bool success;
			float x = items[1].toFloat(&success);
			float y = items[2].toFloat(&success);

			QScatterSeries* serie = series[i];
			serie->setName(items[0]);
			serie->append(x, y);
		}
	}

	QChart* chart = new QChart();
	for( QScatterSeries* serie : series )
		chart->addSeries(serie);
	chart->createDefaultAxes();

	expandXYAxisRange(chart, 0.05f);
	return chart;
}

struct WcGraphOptions
{
	QString    input;
	QString    output;
	chart_type type;
	QString    title;
};

static bool parseOptions(WcGraphOptions* opts)
{
	QStringList types;
	for(const char* type_name : CHART_TYPE_NAME)
		types << type_name;

	QCommandLineOption outputOption("output", "if set the graph will be written to the file, "
	                                          "otherwise the grap will be opened as an app.",  "image-file",    QString());
	QCommandLineOption typeOption  ("type",   "graph type to generate",                        types.join(','), QString());
	QCommandLineOption titleOption ("title",  "chart title, optional",                         "string",        QString());

	QCommandLineParser parser;
	parser.setApplicationDescription("Application to generate charts with QChart");
	parser.addHelpOption();
	parser.addVersionOption();
	parser.addPositionalArgument("source", "Source .csv file to use when generating the chart.");
	parser.addOption(outputOption);
	parser.addOption(typeOption);
	parser.addOption(titleOption);

	parser.process(*qApp);

	QStringList args = parser.positionalArguments();
	if(args.size() > 1)
	{
		qInfo("to many inputs!");
		return false;
	}

	if( !args.isEmpty() )
		opts->input = args[0];
	opts->output = parser.value(outputOption);
	opts->type   = chartTypeFromString(parser.value(typeOption));
	opts->title  = parser.value(titleOption);

	if(opts->type == CHART_TYPE_CNT)
	{
		qInfo("%s", qUtf8Printable(QString("invalid chart-type, should be any of [%0]").arg(types.join(", "))));
		return false;
	}

	return true;
}

static QString readInput(QString input)
{
	if( input.isNull() )
	{
		QFile infile;
		infile.open(stdin, QIODevice::ReadOnly);
		return infile.readAll().trimmed();
	}
	else
	{
		QFile infile(input);
		infile.open(QFile::ReadOnly);
		return infile.readAll().trimmed();
	}
}

class WCChartsWin : public QMainWindow
{
	public:
	WCChartsWin() = default;

	void keyPressEvent(QKeyEvent* event)
	{
		if(event->key() == Qt::Key_Escape)
			QCoreApplication::quit();
		else
			QWidget::keyPressEvent(event);
	}
};

static QChart* createChart(const WcGraphOptions& opts)
{
	QChart* chart = [&opts](chart_type type) -> QChart*
	{
		QString file_data = readInput(opts.input);

		switch(opts.type)
		{
			case CHART_TYPE_BAR_VERTICAL:   return createBarChart(file_data, opts.type);
			case CHART_TYPE_BAR_HORIZONTAL: return createBarChart(file_data, opts.type);
			case CHART_TYPE_SCATTER:        return createScatterChart(file_data);
			default:
				Q_ASSERT(false);
		}
		return nullptr;

	}(opts.type);


	chart->legend()->setVisible(true);
	chart->legend()->setAlignment(Qt::AlignRight);

	chart->axisX()->setTitleText("x axis, don't hardcode me!");
	chart->axisY()->setTitleText("y axis, don't hardcode me!");

	return chart;
}

int main( int argc, char** argv )
{
	QApplication a(argc, argv);
	QApplication::setApplicationName("WcGraph");
	QApplication::setApplicationVersion("1.0.0");

	WcGraphOptions opts;
	if( !parseOptions(&opts) )
		return EXIT_FAILURE;

	QChart* chart = createChart(opts);
	if(!opts.title.isNull())
		chart->setTitle(opts.title);

	QChartView* chartView = new QChartView(chart);
	chartView->setRenderHint(QPainter::Antialiasing);
	chartView->resize(1024, 768);

	if(opts.output.isNull())
	{
		chart->setAnimationOptions(QChart::SeriesAnimations);

		WCChartsWin window;
		window.setCentralWidget(chartView);
		window.resize(1024, 768);
		window.show();
		return a.exec();
	}
	else
	{
		chartView->grab().save(opts.output);
		return EXIT_SUCCESS;
	}
}
