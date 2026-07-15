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
#include <QCheckBox>
#include <memory>
#include <chrono>
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

    // Search mode checkboxes
    QHBoxLayout * modeLayout = new QHBoxLayout();
    QCheckBox * fileNameCheck = new QCheckBox("Filename");
    QCheckBox * contentCheck = new QCheckBox("Content");
    fileNameCheck->setChecked(true);
    contentCheck->setChecked(true);
    modeLayout->addWidget(fileNameCheck);
    modeLayout->addWidget(contentCheck);
    layout->addLayout(modeLayout);

    // Default extensions
    QHBoxLayout* extLayout = new QHBoxLayout();
    QCheckBox* txtCheck = new QCheckBox(".txt"); txtCheck->setChecked(true);
    QCheckBox* cppCheck = new QCheckBox(".cpp"); cppCheck->setChecked(true);
    QCheckBox* pyCheck = new QCheckBox(".py"); pyCheck->setChecked(true);
    QCheckBox* hCheck = new QCheckBox(".h"); hCheck->setChecked(true);
    QCheckBox* mdCheck = new QCheckBox(".md"); mdCheck->setChecked(true);
    QCheckBox* jsonCheck = new QCheckBox(".json"); jsonCheck->setChecked(true);
    QCheckBox* xmlCheck = new QCheckBox(".xml"); xmlCheck->setChecked(true);
    QCheckBox* csvCheck = new QCheckBox(".csv"); csvCheck->setChecked(true);
    extLayout->addWidget(txtCheck);
    extLayout->addWidget(cppCheck);
    extLayout->addWidget(pyCheck);
    extLayout->addWidget(hCheck);
    extLayout->addWidget(mdCheck);
    extLayout->addWidget(jsonCheck);
    extLayout->addWidget(xmlCheck);
    extLayout->addWidget(csvCheck);
    layout->addLayout(extLayout);

    // Custom extensions
    QLineEdit* customExtInput = new QLineEdit();
    customExtInput->setPlaceholderText("Custom extensions (e.g. .c .rs)");
    layout->addWidget(customExtInput);

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
            scanner.setSearchMode(fileNameCheck->isChecked(), contentCheck->isChecked());

            // Insert checked default extensions
            std::set<std::string> exts;
            if (txtCheck->isChecked()) 
                exts.insert(".txt");
            if (cppCheck->isChecked()) 
                exts.insert(".cpp");
            if (pyCheck->isChecked())
                exts.insert(".py");
            if (hCheck->isChecked()) 
                exts.insert(".h");
            if (mdCheck->isChecked()) 
                exts.insert(".md");
            if (jsonCheck->isChecked()) 
                exts.insert(".json");
            if (xmlCheck->isChecked()) 
                exts.insert(".xml");
            if (csvCheck->isChecked()) 
                exts.insert(".csv");

            // Parse custom extensions
            QString customText = customExtInput->text().trimmed();
            if (!customText.isEmpty()) {
                for (const QString& ext : customText.split(" ", Qt::SkipEmptyParts)) {
                    QString e = ext.startsWith(".") ? ext : "." + ext;
                    exts.insert(e.toStdString());
                }
            }

            scanner.setExtensions(exts);

            auto start = std::chrono::high_resolution_clock::now();

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
            auto end = std::chrono::high_resolution_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            int count = scanner.getResults().size();
            QMetaObject::invokeMethod(statusLabel, [=]() {
                statusLabel->setText(QString("Done - %1 results found (%2 ms)").arg(count).arg(ms));
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