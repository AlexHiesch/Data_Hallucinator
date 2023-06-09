#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QFileInfo>
#include <QComboBox>
#include <QCheckBox>
#include <QWidgetAction>
#include <QMenu>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_radioOpenai_clicked();
    void on_radioAzureopenai_clicked();
    void on_horizontalSliderTemperature_valueChanged(int value);
    void readCsvFile();
    void onCsvFileDownloaded(QNetworkReply *reply);
    void on_pushButtonSelectColumns_clicked();
    void generateData();
    void saveInputsToFile();
    void loadInputsFromFile();



    void on_plainTextEditTemperature_textChanged();

private:
    QStandardItemModel *csvModel;
    QNetworkAccessManager *networkManager;
    QVector<QCheckBox *> columnCheckBoxes;
    QString getCsvContentFromModel(QStandardItemModel *model, const QString &separator);
    void displayCsvData(const QByteArray &data, const QString &separator);



private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
