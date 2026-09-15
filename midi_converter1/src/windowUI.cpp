#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QPainter>
#include <QPaintEvent>
#include <QIcon>
#include <QColor>
#include "windowUI.h"
#include "x11_helper.h"
#include "globals.h"
#include "globals.h"
#include "globals.h"
#include "globals.h"
#include "globals.h"

MidiApp::MidiApp(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("RobloxMIDI100 - Starry Night");
    resize(850, 380);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(10);

    const char *asciiArt = R"AA_LOGO(
______    _______  _______  ___      _______  __   __  __   __  ___   ______   ___       __   __        ____         _______        _______ 
|    _ |  |       ||  _    ||   |    |       ||  |_|  ||  |_|  ||   | |      | |   |     |  | |  |      |    |       |  _    |      |  _    |
|   | ||  |   _   || |_|   ||   |    |   _   ||       ||       ||   | |  _    ||   |     |  |_|  |       |   |       | | |   |      | | |   |
|   |_||_ |  | |  ||       ||   |    |  | |  ||       ||       ||   | | | |   ||   |     |       |       |   |       | | |   |      | | |   |
|    __  ||  |_|  ||  _   | |   |___ |  |_|  | |     | |       ||   | | |_|   ||   |     |       | ___   |   |  ___  | |_|   | ___  | |_|   |
|   |  | ||       || |_|   ||       ||       ||   _   || ||_|| ||   | |       ||   |      |     | |   |  |   | |   | |       ||   | |       |
|___|  |_||_______||_______||_______||_______||__| |__||_|   |_||___| |______| |___|       |___|  |___|  |___| |___| |_______||___| |_______|
)AA_LOGO";

    QLabel *logoLabel = new QLabel(this);
    logoLabel->setText(QString::fromUtf8(asciiArt));
    logoLabel->setObjectName("AsciiLogo");
    mainLayout->addWidget(logoLabel);

    // 1. MIDI Dev
    auto *deviceTitle = new QLabel("MIDI Device Configuration", this);
    deviceTitle->setStyleSheet("color: #ffffff; font-weight: bold; margin-top: 4px; font-size: 12px;");
    mainLayout->addWidget(deviceTitle);

    auto *subLayout = new QHBoxLayout();
    portCombo = new QComboBox(this);
    refreshBtn = new QPushButton("⟳", this);
    refreshBtn->setFixedWidth(35);
    subLayout->addWidget(portCombo);
    subLayout->addWidget(refreshBtn);
    mainLayout->addLayout(subLayout);

    connectBtn = new QPushButton("Connect Device", this);
    mainLayout->addWidget(connectBtn);

    // 2. Status
    auto *monitorTitle = new QLabel("Live Monitor", this);
    monitorTitle->setStyleSheet("color: #ffffff; font-weight: bold; margin-top: 8px; font-size: 12px;");
    mainLayout->addWidget(monitorTitle);

    auto *row1 = new QHBoxLayout();
    row1->addWidget(new QLabel("Connection:", this));
    statusLabel = new QLabel("Disconnected", this);
    statusLabel->setStyleSheet("color: #f87171; font-weight: bold;");
    row1->addWidget(statusLabel);
    row1->addStretch();
    mainLayout->addLayout(row1);

    auto *row2 = new QHBoxLayout();
    row2->addWidget(new QLabel("Last Note:", this));
    lastNoteLabel = new QLabel("None", this);
    lastNoteLabel->setStyleSheet("color: #60a5fa; font-weight: bold;");
    row2->addWidget(lastNoteLabel);
    row2->addStretch();

    row2->addWidget(new QLabel("Sustain Pedal:", this));
    pedalStatusLabel = new QLabel("OFF", this);
    pedalStatusLabel->setStyleSheet("color: #a1a1aa; font-weight: bold;");
    row2->addWidget(pedalStatusLabel);
    mainLayout->addLayout(row2);

    updatePorts();

    connect(refreshBtn, &QPushButton::clicked, this, &MidiApp::updatePorts);
    connect(connectBtn, &QPushButton::clicked, this, &MidiApp::connectPort);
}

MidiApp::~MidiApp()
{
    if (midiIn) {
        midiIn->closePort();
        delete midiIn;
    }
}

void MidiApp::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0));

    // moon
    painter.setBrush(QColor(240, 248, 255));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(740, 30, 45, 45);

    // star
    painter.setPen(QColor(255, 255, 255));
    const QPoint stars[] = {
        {45, 25}, {110, 75}, {195, 35}, {305, 85}, {415, 20},
        {515, 65}, {635, 25}, {810, 95}, {75, 145}, {175, 215},
        {275, 155}, {395, 225}, {475, 175}, {595, 235}, {675, 175},
        {820, 215}, {25, 275}, {145, 335}, {245, 285}, {345, 345},
        {445, 295}, {545, 325}, {645, 285}, {765, 335}, {350, 110},
        {550, 120}, {220, 100}, {700, 140}
    };
    for (const auto &star : stars) {
        painter.drawPoint(star);
    }
}

void MidiApp::updatePorts()
{
    portCombo->clear();
    if (!midiIn) {
        try {
            midiIn = new RtMidiIn();
        } catch (...) {
            return;
        }
    }

    unsigned int nPorts = midiIn->getPortCount();
    if (nPorts == 0) {
        portCombo->addItem("No MIDI devices");
        connectBtn->setEnabled(false);
    } else {
        for (unsigned int i = 0; i < nPorts; ++i) {
            portCombo->addItem(QString::fromStdString(midiIn->getPortName(i)));
        }
        connectBtn->setEnabled(true);
    }
}

void MidiApp::connectPort()
{
    int index = portCombo->currentIndex();
    if (index < 0 || midiIn->getPortCount() == 0) return;

    try {
        midiIn->closePort();
        midiIn->openPort(index);
        midiIn->setCallback(&midiCallback);
        midiIn->ignoreTypes(false, false, false);
        statusLabel->setText("Connected");
        statusLabel->setStyleSheet("color: #34d399; font-weight: bold;");
    } catch (const RtMidiError &e) {
        statusLabel->setText("Failed");
        statusLabel->setStyleSheet("color: #f87171; font-weight: bold;");
        e.printMessage();
    }
}

