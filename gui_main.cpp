#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QListWidget>
#include <QLabel>
#include <QMessageBox>
#include <QThread>
#include <memory>
#include "FastScanner.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("FastScanner");
    window.resize(700, 500);

    QVBoxLayout* layout = new QVBoxLayout(&window);

    // Directory input field
    QHBoxLayout* dirLayout = new QHBoxLayout();
    QLineEdit* dirInput = new QLineEdit();
    dirInput->setPlaceholderText("Target Directory...");
    QPushButton* browseBtn = new QPushButton("Browse");
    dirLayout->addWidget(dirInput);
    dirLayout->addWidget(browseBtn);
    layout->addLayout(dirLayout);

    // Keyword input field
    QLineEdit* keywordInput = new QLineEdit();
    keywordInput->setPlaceholderText("Search Keyword...");
    layout->addWidget(keywordInput);

    // Search button
    QPushButton* searchBtn = new QPushButton("Search");
    layout->addWidget(searchBtn);

    // Result count
    QLabel* statusLabel = new QLabel("Ready");
    layout->addWidget(statusLabel);

    // Result list
    QListWidget* resultList = new QListWidget();
    layout->addWidget(resultList);

    // Browse function
    QObject::connect(browseBtn, &QPushButton::clicked, [&]() {
        QString dir = QFileDialog::getExistingDirectory(&window, "Select Directory");
        if (!dir.isEmpty())
            dirInput->setText(dir);
    });

    // Search function
    QObject::connect(searchBtn, &QPushButton::clicked, [&]() {
     QString dir = dirInput->text();
     QString keyword = keywordInput->text();

    if (dir.isEmpty() || keyword.isEmpty()) {
         QMessageBox::warning(&window, "Warning", "Please enter both directory and keyword.");
        return;
    }

    resultList->clear();
    searchBtn->setEnabled(false);
    statusLabel->setText("Scanning...");

    // Scan in seperate thread
    QThread* thread = QThread::create([=, &window]() {
        try {
            FastScanner scanner(keyword.toStdString());

            // Callback: worker thread -> UI thread
            scanner.setOnResultFound([=](const SearchResult& result) {
                QString text;
                if (result.type == SearchResult::FileNameMatch)
                    text = QString("File: %1").arg(QString::fromStdString(result.path));

                else
                    text = QString("Found in %1 (Line: %2)").arg(QString::fromStdString(result.path)).arg(result.lineNumber);

                // UI update in main thread
                QMetaObject::invokeMethod(resultList, [=]() {
                    resultList->addItem(text);
                    }, Qt::QueuedConnection);
                });

            scanner.startScan(dir.toStdString());

            // UI update
            int count = scanner.getResults().size();
            QMetaObject::invokeMethod(statusLabel, [=]() {
                statusLabel->setText(QString("Done - %1 results found").arg(count));
                searchBtn->setEnabled(true);
                }, Qt::QueuedConnection);
        }

        catch (const std::exception& e) {
            QString err = QString::fromStdString(e.what());
            QMetaObject::invokeMethod(statusLabel, [=]() {
                statusLabel->setText(QString("Error: %1").arg(err));
                searchBtn->setEnabled(true);
                }, Qt::QueuedConnection);
        }
        });

    QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
        });

    window.show();
    return app.exec();
}