#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProgressDialog>
#include <QDir>
#include <QTextStream>
#include <QRegularExpression>
#include <sstream>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Data Hallucinator");
    ui->radioAzureopenai->click();
    connect(ui->pushButtonPreview, SIGNAL(clicked()), this, SLOT(readCsvFile()));
    csvModel = new QStandardItemModel(this);
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onCsvFileDownloaded(QNetworkReply*)));
    ui->tableView->setModel(csvModel);
    connect(ui->pushButtonSelectColumns, &QPushButton::clicked, this, &MainWindow::on_pushButtonSelectColumns_clicked);
    connect(ui->pushButtonGenerate, SIGNAL(clicked()), this, SLOT(generateData()));
    loadInputsFromFile();
}


void MainWindow::loadInputsFromFile()
{
    // Get the user's home directory
    QString homeDir = QDir::homePath();
    QString filePath = homeDir + "/inputs.txt";  // Use the same filename as in the saveInputsToFile() function

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);

        // Read the inputs from the file and set the UI elements' values
        ui->plainTextEditFileinput->setPlainText(in.readLine().split(": ").last());
        ui->plainTextEditFileoutput->setPlainText(in.readLine().split(": ").last());
        ui->plainTextEditApikey->setPlainText(in.readLine().split(": ").last());
        ui->plainTextEditModel->setPlainText(in.readLine().split(": ").last());
        ui->plainTextEditEndpoint->setPlainText(in.readLine().split(": ").last());
        ui->lineEditSampleoutputsize->setText(in.readLine().split(": ").last());
        ui->plainTextEditTemperature->setPlainText(in.readLine().split(": ").last());
        ui->lineEditSeparatorinput->setText(in.readLine().split(": ").last());
        ui->lineEditSeparatoroutput->setText(in.readLine().split(": ").last());

        file.close();
    } else {
        // Handle file open error or the case when the file does not exist yet
    }
}


void MainWindow::saveInputsToFile()
{
    // Get the user's home directory
    QString homeDir = QDir::homePath();
    QString filePath = homeDir + "/inputs.txt";  // You can change the filename as needed

    // Open the file in WriteOnly mode
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);

        // Write the inputs to the file
        out << "File Input: " << ui->plainTextEditFileinput->toPlainText() << "\n";
        out << "File Output: " << ui->plainTextEditFileoutput->toPlainText() << "\n";
        out << "API Key: " << ui->plainTextEditApikey->toPlainText() << "\n";
        out << "Model: " << ui->plainTextEditModel->toPlainText() << "\n";
        out << "Endpoint: " << ui->plainTextEditEndpoint->toPlainText() << "\n";
        out << "Sample Size: " << ui->lineEditSampleoutputsize->text() << "\n";
        out << "Temperature: " << ui->plainTextEditTemperature->toPlainText() << "\n";
        out << "Separator (input): " << ui->lineEditSeparatorinput->text() << "\n";
        out << "Separator (output): " << ui->lineEditSeparatoroutput->text() << "\n";

        file.close();
    } else {
        // Handle file open error
    }
}


MainWindow::~MainWindow()
{
    delete ui;
}

std::vector<std::string> readCSVRow(const std::string &row, const QString &separator) {
    std::istringstream rowStream(row);
    std::vector<std::string> rowData;
    std::string cell;
    std::string sep = separator.toStdString();

    while (std::getline(rowStream, cell, sep[0])) {
        rowData.push_back(cell);
    }

    return rowData;
}



void MainWindow::displayCsvData(const QByteArray &data, const QString &separator)
{
    QTextStream textStream(data);
    QString content = textStream.readAll();

    std::istringstream inputStream(content.toStdString());
    std::vector<std::vector<std::string>> rows;
    std::string row_str;
    while (std::getline(inputStream, row_str)) {
        rows.push_back(readCSVRow(row_str, separator));
    }



    // Check if the rows vector is not empty before accessing its elements
    if (!rows.empty()) {
        // Create a new QStandardItemModel and set the header data
        delete csvModel;
        csvModel = new QStandardItemModel(rows.size() - 1, rows[0].size(), this);
        for (int i = 0; i < rows[0].size(); ++i) {
            csvModel->setHeaderData(i, Qt::Horizontal, QString::fromStdString(rows[0][i]));
        }

        // Insert the rows into the model
        rows.erase(rows.begin());
        for (int rowIdx = 0; rowIdx < rows.size(); ++rowIdx) {
            const std::vector<std::string> &row = rows[rowIdx];
            for (int colIdx = 0; colIdx < row.size(); ++colIdx) {
                csvModel->setItem(rowIdx, colIdx, new QStandardItem(QString::fromStdString(row[colIdx])));
            }
        }

        ui->tableView->setModel(csvModel); // Set the model for the tableView
    } else {
        // Handle the case when the rows list is empty
    }
}


QString MainWindow::getCsvContentFromModel(QStandardItemModel *model, const QString &separator)
{
    QString csvContent;
    for (int row = 0; row < model->rowCount(); ++row) {
        for (int col = 0; col < model->columnCount(); ++col) {
            QStandardItem *item = model->item(row, col);
            if (item) {  // Make sure the item is valid before accessing its text
                csvContent += item->text();
            }
            if (col < model->columnCount() - 1) {
                csvContent += separator;
            }
        }
        csvContent += "\n";
    }
    return csvContent;
}



void MainWindow::generateData()
{
    // Show a progress dialog while the API call and file output are in progress
    QProgressDialog *progressDialog = new QProgressDialog("Loading...", "Cancel", 0, 0, this);
    progressDialog->setMinimumDuration(0);
    progressDialog->setWindowTitle("Please Wait");
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->show();

    // Get values from UI elements
    QString endpoint = ui->plainTextEditEndpoint->toPlainText().trimmed();
    QString apiKey = ui->plainTextEditApikey->toPlainText().trimmed();
    QString csvContent = getCsvContentFromModel(csvModel, ui->lineEditSeparatoroutput->text());
    int sampleSize = ui->lineEditSampleoutputsize->text().toInt();
    QString separator = ui->lineEditSeparatoroutput->text();
    double temperature = ui->plainTextEditTemperature->toPlainText().toDouble();

    // Prepare the JSON request body
    QJsonObject requestBody;
    QJsonArray messages;
    QJsonObject messageSystem;
    messageSystem["role"] = "system";
    messageSystem["content"] = csvContent;
    messages.append(messageSystem);

    QJsonObject messageUser;
    messageUser["role"] = "user";
    messageUser["content"] = QString("Please generate from provided data new %1 numbers of row as csv with %2 as the separator including the header row and make sure provided data is not present and randomize as much as possible with removing personalized data. Only output data without any comments from your side.").arg(sampleSize).arg(separator);
    messages.append(messageUser);

    requestBody["messages"] = messages;
    requestBody["max_tokens"] = 800;
    requestBody["temperature"] = temperature;
    requestBody["frequency_penalty"] = 0;
    requestBody["presence_penalty"] = 0;
    requestBody["top_p"] = 0.95;
    requestBody["stop"] = QJsonValue::Null;

    QJsonDocument jsonDoc(requestBody);

    // Set up a QNetworkRequest with the necessary headers
    QNetworkRequest request;
    request.setUrl(QUrl(endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("api-key", apiKey.toUtf8());

    // Send a POST request using the QNetworkAccessManager
    QNetworkReply *reply = networkManager->post(request, jsonDoc.toJson());
    // Close the progress dialog when the API call finishes
    connect(reply, &QNetworkReply::finished, progressDialog, &QProgressDialog::close);
    saveInputsToFile();
}



void MainWindow::readCsvFile()
{
    QString fileInput = ui->plainTextEditFileinput->toPlainText().trimmed();
    QString inputSeparator = ui->lineEditSeparatorinput->text(); // Rename the variable
    QUrl url(fileInput);
    if (url.isValid() && url.scheme().startsWith("http")) {
        // Download the CSV file from the URL
        QNetworkRequest request(url);
        QNetworkReply *reply = networkManager->get(request);
        // Connect the finished signal of the reply to the onCsvFileDownloaded slot
        connect(reply, &QNetworkReply::finished, this, [=]() {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray data = reply->readAll();
                displayCsvData(data, inputSeparator);
            } else {
                // Handle error
            }
            reply->deleteLater();
        });
    } else {
        // Read the CSV file from the local file path
        QFileInfo fileInfo(fileInput);
        if (fileInfo.exists() && fileInfo.isFile()) {
            QFile file(fileInput);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray data = file.readAll();
                file.close();
                displayCsvData(data, inputSeparator);
            } else {
                // Handle file open error
            }
        } else {
            // Handle invalid file path error
        }
    }
}


void MainWindow::on_pushButtonSelectColumns_clicked()
{
    qDebug() << "Select Columns button clicked!";
    QMenu menu(this);

    for (int i = 0; i < columnCheckBoxes.size(); ++i) {
        QAction *action = menu.addAction(csvModel->headerData(i, Qt::Horizontal).toString());
        action->setCheckable(true);
        action->setChecked(!ui->tableView->isColumnHidden(i));

        connect(action, &QAction::toggled, [=](bool checked) {
            ui->tableView->setColumnHidden(i, !checked);
        });
    }

    menu.exec(QCursor::pos());
}



void MainWindow::onCsvFileDownloaded(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QString inputSeparator = ui->lineEditSeparatorinput->text(); // Rename the variable
        displayCsvData(data, inputSeparator); // Pass the renamed variable
        // Parse the response JSON to extract the generated data
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        QJsonObject jsonObject = jsonDoc.object();
        QString generatedData = jsonObject["choices"].toArray().first().toObject()["message"].toObject()["content"].toString();

        // Save the generated data to a CSV file
        QString fileOutput = ui->plainTextEditFileoutput->toPlainText().trimmed();
        QFile file(fileOutput);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << generatedData;
            file.close();
        } else {
            // Handle file open error
        }
    } else {
        // Handle error
    }
    reply->deleteLater();
}




void MainWindow::on_radioOpenai_clicked()
{
    ui->labelEndpoint->setVisible(0);
    ui->plainTextEditEndpoint->setVisible(0);
    ui->labelModel->setVisible(1);
    ui->plainTextEditModel->setVisible(1);

}


void MainWindow::on_radioAzureopenai_clicked()
{

    ui->labelEndpoint->setVisible(1);
    ui->plainTextEditEndpoint->setVisible(1);
    ui->labelModel->setVisible(0);
    ui->plainTextEditModel->setVisible(0);
}


void MainWindow::on_horizontalSliderTemperature_valueChanged(int value)
{
    double casted_value = static_cast<double>(value) / 100;
    QString s = QString::number(casted_value, 'f', 1);

    ui->plainTextEditTemperature->setPlainText(s);
}


void MainWindow::on_plainTextEditTemperature_textChanged()
{
    double casted_value = ui->plainTextEditTemperature->toPlainText().toDouble() * 100;
    int slider_value = static_cast<int>(casted_value);
    ui->horizontalSliderTemperature->setValue(slider_value);
}
