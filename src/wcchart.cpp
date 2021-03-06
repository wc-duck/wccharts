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
#include <QtCharts/QBarSet>
#include <QtCharts/QLegend>
#include <QtCharts/QBarCategoryAxis>

#include <QtCharts/QScatterSeries>

QT_CHARTS_USE_NAMESPACE

static QBarSeries* readCSVHeader( const QString& line )
{
	QStringList types = line.split(',');
	types.pop_front();

	QBarSeries* series = new QBarSeries();
	for( QString& type : types )
		series->append(new QBarSet(type));
	return series;
}

static QString parseCSVLine( const QString& line, QList<QBarSet*>& sets )
{
	QStringList items = line.split(',');
	QString category = items.takeFirst();

	bool success;
	for( int i = 0; i < items.size(); ++i )
		sets[i]->append(items[i].toFloat(&success));

	return category;
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

static QChart* createBarChart( const QString& csv_data )
{
	QStringList lines = csv_data.split('\n');

	QStringList categories;

	QBarSeries* series = readCSVHeader(lines.takeFirst());
	QList<QBarSet*> sets = series->barSets();

	while(!lines.empty())
		categories << parseCSVLine( lines.takeFirst(), sets );

	QChart* chart = new QChart();
	chart->addSeries(series);

	QBarCategoryAxis *axis = new QBarCategoryAxis();
	axis->append(categories);
	chart->createDefaultAxes();
	chart->setAxisX(axis, series);

	chart->legend()->setVisible(true);
	chart->legend()->setAlignment(Qt::AlignRight);

	expandYAxisRange(chart, 0.05f);

	return chart;
}

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

	chart->legend()->setVisible(true);
	chart->legend()->setAlignment(Qt::AlignRight);

	expandXYAxisRange(chart, 0.05f);
	chart->axisX()->setTitleText("bytes/codepoint");
	chart->axisY()->setTitleText("time (ms)/10000 codepoints");

	return chart;
}

struct WcGraphOptions
{
	QString input;
	QString output;
	QString type;
	QString title;
};

static bool parseOptions(WcGraphOptions* opts)
{
	QCommandLineOption outputOption("output", "if set the graph will be written to the file, otherwise the grap will be opened as an app.", "image-file",   QString());
	QCommandLineOption typeOption  ("type",   "graph type to generate",                                                                     "bar, scatter", QString());
	QCommandLineOption titleOption ("title",  "chart title, optional",                                                                      "string",       QString());

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
	opts->type   = parser.value(typeOption);
	opts->title  = parser.value(titleOption);

	if(opts->type != "bar" && opts->type != "scatter")
	{
		qInfo("invalid chart-type, should be 'bar' or 'scatter'");
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

int main( int argc, char** argv )
{
	QApplication a(argc, argv);
	QApplication::setApplicationName("WcGraph");
	QApplication::setApplicationVersion("1.0.0");

	WcGraphOptions opts;
	if( !parseOptions(&opts) )
		return EXIT_FAILURE;

	QString file_data = readInput(opts.input);

	QChart* chart = nullptr;
	if( opts.type == "bar" )     chart = createBarChart(file_data);
	if( opts.type == "scatter" ) chart = createScatterChart(file_data);

	if(!opts.title.isNull())
		chart->setTitle(opts.title);

	QChartView* chartView = new QChartView(chart);
	chartView->setRenderHint(QPainter::Antialiasing);
	chartView->resize(1024, 768);

	if(opts.output.isNull())
	{
		chart->setAnimationOptions(QChart::SeriesAnimations);

		QMainWindow window;
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
