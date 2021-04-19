#include "mainwindow.h"
#include "app.h"
#include "ui_mainwindow.h"
#include "ui_datadialog.h"
#include "ergen.h"
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <QDialog>
#include <QFontDatabase>
#include <QTimer>
#include <QDebug>

struct MainWindow::Impl {
    MainWindow* self_ = nullptr;
    QTimer* regenTimer_ = nullptr;
    Ui::MainWindow ui_;
    ERgen::PRNG prng_;

    QDialog* dataDialog_ = nullptr;
    Ui::DataDialog uiDataDialog_;
    QString dataText_;

    QDialog* faustDialog_ = nullptr;
    Ui::DataDialog uiFaustDialog_;
    QString faustText_;

    ///
    void regenerate();
    void regenerateLater();
    void setupCurves(const ERgen& leftGen, const ERgen& rightGen, double timeRange);
    void setBypassed(bool bypassed);
};

///
static constexpr double kMinRandomSpead = 0.01;

///
static const QColor fadeColors[2] = {Qt::green, Qt::magenta};
static const QColor tapsColors[2] = {Qt::blue, Qt::red};

///
MainWindow::MainWindow()
    : impl_(new Impl)
{
    Impl& impl = *impl_;
    Ui::MainWindow& ui = impl.ui_;

    impl.self_ = this;

    ui.setupUi(this);

    connect(ui.leftSeedSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, [this]() { impl_->regenerateLater(); });
    connect(ui.leftFadeOutKnob, &QwtKnob::valueChanged, this, [this]() { impl_->regenerateLater(); });
    connect(ui.leftRandomSpreadKnob, &QwtKnob::valueChanged, this, [this]() { impl_->regenerateLater(); });
    connect(ui.leftTapCountKnob, &QwtKnob::valueChanged, this, [this]() { impl_->regenerateLater(); });
    connect(ui.leftGainSpreadKnob, &QwtKnob::valueChanged, this, [this]() { impl_->regenerateLater(); });

    connect(ui.rightSeedSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, [this]() { impl_->regenerateLater(); });
    connect(ui.rightFadeOutKnob, &QwtKnob::valueChanged, this, [this]() { impl_->regenerateLater(); });
    connect(ui.rightRandomSpreadKnob, &QwtKnob::valueChanged, this, [this]() { impl_->regenerateLater(); });
    connect(ui.rightTapCountKnob, &QwtKnob::valueChanged, this, [this]() { impl_->regenerateLater(); });
    connect(ui.rightGainSpreadKnob, &QwtKnob::valueChanged, this, [this]() { impl_->regenerateLater(); });

    connect(ui.rangeKnob, &QwtKnob::valueChanged, this, [this]() { impl_->regenerateLater(); });

    connect(ui.dataButton, &QAbstractButton::clicked, this, [this]() { impl_->dataDialog_->show(); });
    connect(ui.faustButton, &QAbstractButton::clicked, this, [this]() { impl_->faustDialog_->show(); });

    connect(ui.bypassButton, &QAbstractButton::toggled, this, [this](bool value) { impl_->setBypassed(value); });

    QLabel* headLabels[2] = {ui.leftHeadLabel, ui.rightHeadLabel};
    for (int channel = 0; channel < 2; ++channel) {
        QPalette palette = headLabels[channel]->palette();
        palette.setColor(headLabels[channel]->foregroundRole(), tapsColors[channel]);
        headLabels[channel]->setPalette(palette);
    }

    ///
    QDialog* dataDialog = new QDialog(this);
    impl.dataDialog_ = dataDialog;
    impl.uiDataDialog_.setupUi(dataDialog);
    dataDialog->setWindowTitle(tr("Data"));
    QTextBrowser* dataTextBrowser = impl.uiDataDialog_.textBrowser;
    dataTextBrowser->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    ///
    QDialog* faustDialog = new QDialog(this);
    impl.faustDialog_ = faustDialog;
    impl.uiFaustDialog_.setupUi(faustDialog);
    faustDialog->setWindowTitle(tr("Faust"));
    QTextBrowser* faustTextBrowser = impl.uiFaustDialog_.textBrowser;
    faustTextBrowser->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    ///
    impl.regenerateLater();
}

void MainWindow::Impl::regenerate()
{
    Ui::MainWindow& ui = ui_;

    ERgen::Setup leftSetup;
    leftSetup.fade = ui.leftFadeOutKnob->value();
    leftSetup.rho = std::max(kMinRandomSpead, ui.leftRandomSpreadKnob->value());
    leftSetup.numTaps = (int)ui.leftTapCountKnob->value();
    leftSetup.gainSpread = ui.leftGainSpreadKnob->value();
    leftSetup.numPoints = 256;
    leftSetup.prng = &prng_;
    leftSetup.prng->seed(ui.leftSeedSpinBox->value());

    ERgen leftGen(leftSetup);
    leftGen.calc();

    ERgen::Setup rightSetup;
    rightSetup.fade = ui.rightFadeOutKnob->value();
    rightSetup.rho = std::max(kMinRandomSpead, ui.rightRandomSpreadKnob->value());
    rightSetup.numTaps = (int)ui.rightTapCountKnob->value();
    rightSetup.gainSpread = ui.rightGainSpreadKnob->value();
    rightSetup.numPoints = 256;
    rightSetup.prng = &prng_;
    rightSetup.prng->seed(65537-ui.rightSeedSpinBox->value());

    ERgen rightGen(rightSetup);
    rightGen.calc();

    double timeRange = 1e-3 * ui.rangeKnob->value();

    setupCurves(leftGen, rightGen, timeRange);
}

void MainWindow::Impl::regenerateLater()
{
    QTimer* tim = regenTimer_;
    if (!tim) {
        MainWindow* self = self_;
        tim = new QTimer(self);
        tim->setInterval(20);
        tim->setSingleShot(true);
        connect(tim, &QTimer::timeout, self, [this]() { regenerate(); });
        regenTimer_ = tim;
    }
    tim->start();
}

void MainWindow::Impl::setupCurves(const ERgen& leftGen, const ERgen& rightGen, double timeRange)
{
    Ui::MainWindow& ui = ui_;
    QwtPlot* plot = ui.plot;
    const ERgen* LRgen[2] = {&leftGen, &rightGen};

    int numPoints = LRgen[0]->getNumPoints();
    Q_ASSERT(numPoints == LRgen[1]->getNumPoints());

    int numLeftTaps = LRgen[0]->getNumTaps();
    int numRightTaps = LRgen[1]->getNumTaps();

    plot->detachItems();

    ///
    plot->setCanvasBackground(Qt::black);

    ///
    plot->setAxisScale(QwtPlot::xBottom, 0.0, 1.0);
    plot->setAxisScale(QwtPlot::yLeft, 0.0, 1.0);

    ///
    QwtPlotGrid *grid = new QwtPlotGrid;
    grid->setPen(Qt::gray, 0.0, Qt::DotLine);
    grid->attach(plot);

    ///
    for (int channel = 0; channel < 2; ++channel) {
        const ERgen& gen = *LRgen[channel];

        QVector<double> fadePointsX(numPoints);
        QVector<double> fadePointsY(numPoints);
        for (int i = 0; i < numPoints; ++i) {
            double x = i / (double)(numPoints - 1);
            double y = gen.calcFadeGain(x);
            fadePointsX[i] = x;
            fadePointsY[i] = y;
        }

        ///
        QwtPlotCurve* fadeCurve = new QwtPlotCurve(tr("Fade"));
        fadeCurve->setPen(fadeColors[channel], 0.0, Qt::DotLine);
        fadeCurve->setSamples(fadePointsX.data(), fadePointsY.data(), numPoints);
        fadeCurve->attach(plot);
    }

    ///
    for (int channel = 0; channel < 2; ++channel) {
        const ERgen& gen = *LRgen[channel];

        const double* positions = gen.getPositions();
        const double* gains = gen.getGains();

        QwtPlotCurve* tapsCurve = new QwtPlotCurve(tr("Taps"));
        tapsCurve->setStyle(QwtPlotCurve::Sticks);
        tapsCurve->setPen(tapsColors[channel], 0.0, Qt::SolidLine);
        tapsCurve->setSamples(positions, gains, gen.getNumTaps());
        tapsCurve->attach(plot);
    }

    ///
    plot->updateAxes();
    plot->replot();

    ///
    const double* leftPositions = LRgen[0]->getPositions();
    const double* rightPositions = LRgen[1]->getPositions();
    const double* leftGains = LRgen[0]->getGains();
    const double* rightGains = LRgen[1]->getGains();

    static_cast<Application*>(qApp)->setStereoER(leftPositions, rightPositions, leftGains, rightGains, numLeftTaps, numRightTaps, timeRange);

    ///
    {

        QByteArray textByteArray;
        textByteArray.reserve(8192);

        QTextStream textStream(&textByteArray);
        int fieldWidth = 16;
        textStream.setFieldWidth(fieldWidth);
        textStream << ";; Left delay" << "Left gain"
                   << "Right delay" << "Right gain";
        textStream.setFieldWidth(0);
        textStream << '\n';
        for (int i = 0; i < std::max(numLeftTaps, numRightTaps); ++i) {
            textStream.setFieldWidth(fieldWidth);

            if (i < numLeftTaps)
                textStream << (timeRange * leftPositions[i]) << leftGains[i];
            else
                textStream << '_' << '_';

            if (i < numRightTaps)
                textStream << (timeRange * rightPositions[i]) << rightGains[i];
            else
                textStream << '_' << '_';

            textStream.setFieldWidth(0);
            textStream << '\n';
        }
        textStream.flush();

        dataText_ = QString::fromUtf8(textByteArray);

        uiDataDialog_.textBrowser->setPlainText(dataText_);
    }

    ///
    {
        QByteArray textByteArray;
        textByteArray.reserve(8192);

        QTextStream textStream(&textByteArray);

        textStream << "import(\"stdfaust.lib\");\n\n";

        textStream << "process = leftER, rightER;\n\n";

        textStream << "leftER(x) = sum(i, ntaps, g(i)*(x@(d(i)*ma.SR))) with {\n"
            "  ntaps = " << numLeftTaps << ";\n";
        for (int i = 0; i < numLeftTaps; ++i) {
            textStream << "  d(" << i << ") = " << (timeRange * leftPositions[i])
                       << "; g(" << i << ") = " << leftGains[i] << ";\n";
        }
        textStream << "};\n";

        textStream << "\n";

        textStream << "rightER(x) = sum(i, ntaps, g(i)*(x@(d(i)*ma.SR))) with {\n"
            "  ntaps = " << numRightTaps << ";\n";
        for (int i = 0; i < numRightTaps; ++i) {
            textStream << "  d(" << i << ") = " << (timeRange * rightPositions[i])
                       << "; g(" << i << ") = " << rightGains[i] << ";\n";
        }
        textStream << "};\n";

        textStream.flush();

        faustText_ = QString::fromUtf8(textByteArray);

        uiFaustDialog_.textBrowser->setPlainText(faustText_);
    }
}

void MainWindow::Impl::setBypassed(bool bypassed)
{
    static_cast<Application*>(qApp)->setBypassed(bypassed);
}
