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
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <memory>
#include <chrono>
#include "FastScanner.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // UI design
    app.setStyleSheet(R"(
    QWidget {
        background-color: #FAFAFA;
        font-family: "Segoe UI", sans-serif;
        font-size: 15px;
        color: #202020;
    }

    QLineEdit {
        background-color: #FFFFFF;
        border: 1px solid #E0E0E0;
        border-bottom: 2px solid #ACACAC;
        border-radius: 4px;
        padding: 6px 10px;
        font-size: 15px;
    }

    QLineEdit:focus {
        border-bottom: 2px solid #0078D4;
    }

    QPushButton {
        background-color: #0078D4;
        color: #FFFFFF;
        border: none;
        border-radius: 4px;
        padding: 6px 16px;
        font-size: 15px;
    }

    QPushButton:hover {
        background-color: #1A86D9;
    }

    QPushButton:pressed {
        background-color: #006CBE;
    }

    QPushButton:disabled {
        background-color: #F0F0F0;
        color: #A0A0A0;
        border: 1px solid #E0E0E0;
    }

    QCheckBox {
        spacing: 6px;
        font-size: 15px;
        color: #1D1D1F;
    }

    QListWidget {
        background-color: #FFFFFF;
        border: 1px solid #E0E0E0;
        border-radius: 4px;
        padding: 2px;
        font-size: 12px;
    }

    QListWidget::item {
        padding: 4px 8px;
        border-radius: 3px;
    }

    QListWidget::item:hover {
        background-color: #F5F5F5;
    }

    QListWidget::item:selected {
        background-color: #E5F1FB;
        color: #202020;
    }

    QLabel {
        font-size: 12px;
        color: #606060;
        background-color: transparent;
    }
)");

    QWidget window;
    window.setWindowTitle("FastScanner");
    window.resize(700, 500);

    QVBoxLayout* layout = new QVBoxLayout(&window);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    // Directory input field
    QHBoxLayout* dirLayout = new QHBoxLayout();
    QLineEdit* dirInput = new QLineEdit();
    dirInput->setPlaceholderText("Target Directory...");
    QPushButton* browseBtn = new QPushButton("Browse");
    browseBtn->setStyleSheet(R"(
    QPushButton {
        background-color: #F0F0F0;
        color: #202020;
        border: 1px solid #E0E0E0;
    }
    QPushButton:hover {
        background-color: #E5E5E5;
    }
)");

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
    QHBoxLayout* modeLayout = new QHBoxLayout();
    QCheckBox* fileNameCheck = new QCheckBox("Filename");
    QCheckBox* contentCheck = new QCheckBox("Content");
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
    resultList->setContextMenuPolicy(Qt::CustomContextMenu);

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

        // Scan in separate thread
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
                int files = scanner.getScannedFileCount();
                QMetaObject::invokeMethod(statusLabel, [=]() {

                    statusLabel->setText(QString("Done - %1 matches in %2 files (%3 ms)").arg(count).arg(files).arg(ms));
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

    // Actions with right click 
    QObject::connect(resultList, &QListWidget::customContextMenuRequested, [=](const QPoint& pos) {
        QListWidgetItem* item = resultList->itemAt(pos);
        if (!item) // Clicking empty space
            return;

        QMenu menu;
        QAction* openFile = menu.addAction("Open File");
        QAction* openFolder = menu.addAction("Open in Explorer  ");
        QAction* copyPath = menu.addAction("Copy Path");
        QAction* selected = menu.exec(resultList->mapToGlobal(pos));

        // Extract path from result text
        QString text = item->text();
        QString path;
        if (text.startsWith("File: "))
            path = text.mid(6); // Trim "File: "
        else if (text.startsWith("Found in "))
            path = text.mid(9, text.indexOf(" (Line:") - 9); // Trim "Found in " & " (Line:"

        // Open file
        if (selected == openFile && !path.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            
        }

        // Open folder
        else if (selected == openFolder && !path.isEmpty()) {
#ifdef _WIN32
            QProcess::startDetached("explorer", { "/select,", QDir::toNativeSeparators(path) });
#else
            QProcess::startDetached("open", { "-R", path });
#endif
        }

        else if (selected == copyPath && !path.isEmpty()) {
            QApplication::clipboard()->setText(path);
        }
        });

    window.show();
    return app.exec();
}