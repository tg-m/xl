#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <qfiledialog.h>
#include <QDir>

#include "logstatus.h"
#include "../include/xl/log.h"
using namespace xl::log;



MainWindow::MainWindow(QStringList filenames, QWidget *parent) :
    QMainWindow(parent),
    ui(std::make_unique<Ui::MainWindow>())
{
    ui->setupUi(this);
    auto tabs = ui->logStatusTabs;
    while(tabs->count() > 0) {
        tabs->removeTab(0);
    }


    for(auto const & filename : filenames) {
        // if it's a directory, then load up all the .log_status files in it
        QDir dir(filename);
        if (dir.exists()) {
            for (auto entry : dir.entryInfoList({"*.log_status"})) {
                std::cerr << fmt::format("Looking at filename: {} - {}", entry.canonicalFilePath().toStdString(),
                                         entry.fileName().toStdString()) << std::endl;
                tabs->addTab(new LogStatus(entry.canonicalFilePath()), entry.fileName());
            }
        } else {
            tabs->addTab(new LogStatus(filename), filename);
        }
    }
}

MainWindow::~MainWindow() = default;



void MainWindow::on_action_Open_triggered()
{
    auto filename = QFileDialog::getOpenFileName(this,
        tr("Open Status File"), ".", tr("Image Files (*.log_status)")
    );

    this->ui->logStatusTabs->addTab(new LogStatus(filename), filename);

}

















