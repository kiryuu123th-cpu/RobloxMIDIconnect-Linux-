#pragma once

#include <QWidget>

class MidiApp : public QWidget {
    Q_OBJECT
public:
    explicit MidiApp(QWidget *parent = nullptr);
    ~MidiApp();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updatePorts();
    void connectPort();

private:
    class QComboBox *portCombo;
    class QPushButton *refreshBtn;
    class QPushButton *connectBtn;
};
