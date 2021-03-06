#include "dialogrecovery.h"
#include "ui_dialogrecovery.h"
#include <paths.h>

DialogRecovery::DialogRecovery(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogRecovery)
{
    ui->setupUi(this);
    busy = new bool(false);

    // Configure the table
    ui->tableWidget->setColumnCount(2);
    QStringList m_TableHeader;
    m_TableHeader << "Name" << "Size";
    ui->tableWidget->setHorizontalHeaderLabels(m_TableHeader);
    // Beautification
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    // Get our context menu up and running
    ui->tableWidget->insertAction(NULL,ui->actionDelete);
    ui->tableWidget->insertAction(ui->actionDelete,ui->actionRefresh);

    // We don't want to see the empty progress bar
    ui->progressBar->setVisible(false);
    // Enable the button upon selection only!
    ui->flashButton->setEnabled(false);
    ui->bootButton->setEnabled(false);

    getFiles();

    // Let's check if we have something
    if(ui->tableWidget->rowCount() == 0)
    {
        // Prepare a messagebox
        QMessageBox msgBox(this->parentWidget());
        QPixmap icon(":/Icons/recovery.png");
        msgBox.setIconPixmap(icon);
        msgBox.setText("There are no recoveries available!");
        msgBox.setInformativeText("You can manually add one or wait for a new release.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
    }
}

DialogRecovery::~DialogRecovery()
{
    delete ui;
}

void DialogRecovery::getFiles()
{
    // Reset our row count
    ui->tableWidget->setRowCount(0);

    QDir temp_path(QCoreApplication::applicationDirPath());

#ifdef Q_OS_MACX
    // Because apple likes it's application folders
    temp_path.cdUp();
    temp_path.cdUp();
    temp_path.cdUp();
#endif

    // Let's populate the list
    QDirIterator dirit(QDir::toNativeSeparators(temp_path.absolutePath()+pathRecoveries),QDirIterator::Subdirectories);

    while(dirit.hasNext())
    {
        dirit.next();
        if (QFileInfo(dirit.filePath()).suffix() == "img")
        {
            qint64 size_to_convert = QFileInfo(dirit.filePath()).size();
            float size_converted_float;
            QString size_converted_string;

            // Convert to B, KB or MB
            if (size_to_convert < 1025)
            {
                size_converted_string = QString::number(size_to_convert) + " B";
            }
            else if (size_to_convert < 1048577)
            {
                size_converted_float = size_to_convert / 1024.f;
                size_converted_string = QString::number(size_converted_float, 'f', 3) + " KB";
            }
            else
            {
                size_converted_float = size_to_convert / 1048576.f;
                size_converted_string = QString::number(size_converted_float, 'f', 2) + " MB";
            }

            // Add our stuff to the table
            ui->tableWidget->setRowCount(ui->tableWidget->rowCount()+1);
            ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,0,new QTableWidgetItem(dirit.fileName()));
            ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,1,new QTableWidgetItem(size_converted_string));
        }
    }
}

// Not used right now
void DialogRecovery::processOutput()
{
    QProcess *p = dynamic_cast<QProcess *>( sender() );
    QString progress = p->readAllStandardOutput();

    if(progress.contains("sending"))
        ui->progressBar->setValue(33);

    if(progress.contains("writing"))
        ui->progressBar->setValue(66);

    if(progress.contains("finished"))
        ui->progressBar->setValue(100);
}

void DialogRecovery::processFinished(int exitCode)
{
    QProcess *p = dynamic_cast<QProcess *>( sender() );

    QString error(p->readAllStandardError());
    error.remove("\n");
    error.remove("\r");

    if(exitCode != 0)
    {
        // Prepare a messagebox
        QMessageBox msgBox(this->parentWidget());
        QPixmap icon(":/Icons/recovery.png");
        msgBox.setIconPixmap(icon);
        msgBox.setText("Oups! Something went wrong...");
        msgBox.setInformativeText(error);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
    }
    else
    {
        // Prepare a messagebox
        QMessageBox msgBox(this->parentWidget());
        QPixmap icon(":/Icons/recovery.png");
        msgBox.setIconPixmap(icon);
        msgBox.setText("Flashing finished!");
        msgBox.setInformativeText("Your selected recovery has been flashed successfully.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
    }

    busy = false;

    ui->progressBar->setValue(100);
    ui->tableWidget->setEnabled(true);
    ui->buttonBox->setEnabled(true);
    ui->flashButton->setEnabled(true);
}

void DialogRecovery::on_flashButton_clicked()
{
    if (DeviceConnection::getConnection(DEFAULT_TIMEOUT) == FASTBOOT)
    {
        if(ui->tableWidget->currentItem() != NULL)
        {
            QDir temp_path(QCoreApplication::applicationDirPath());

#ifdef Q_OS_MACX
            // Because apple likes it's application folders
            temp_path.cdUp();
            temp_path.cdUp();
            temp_path.cdUp();
#endif

            QProcess* process_flash = new QProcess(this);
            QString temp_cmd = QDir::toNativeSeparators(temp_path.absolutePath()+pathRecoveries)+ui->tableWidget->item(ui->tableWidget->currentRow() ,0)->text()+"\"\n";
            QString recovery;

            connect( process_flash, SIGNAL(readyReadStandardOutput()), this, SLOT(processOutput()));
            connect( process_flash, SIGNAL(finished(int)), this, SLOT(processFinished(int)));

            // Restrict from closing while flashing
            busy = true;

            process_flash->setWorkingDirectory(QDir::toNativeSeparators(temp_path.absolutePath()+"/Data/"));
#ifdef Q_OS_WIN
            // Windows code here
            process_flash->start("cmd");
            recovery = "fastboot.exe flash recovery \"" + temp_cmd;
#elif defined(Q_OS_MACX)
            // MAC code here
            process_flash->start("sh");
            recovery = "./fastboot_mac flash recovery \"" + temp_cmd;
#else
            // Linux code here
            process_flash->start("sh");
            recovery = "./fastboot_linux flash recovery \"" + temp_cmd;
#endif
            process_flash->waitForStarted();
            process_flash->write(recovery.toLatin1());
            process_flash->write("exit\n");

            // UI restrictions
            ui->tableWidget->setEnabled(false);
            ui->buttonBox->setEnabled(false);
            ui->flashButton->setEnabled(false);
            ui->progressBar->setVisible(true);
            ui->progressBar->setValue(0);
        }
    }
    else
    {
        // Prepare a messagebox
        QMessageBox msgBox(this->parentWidget());
        QPixmap icon(":/Icons/recovery.png");
        msgBox.setIconPixmap(icon);
        msgBox.setText("No connection!");
        msgBox.setInformativeText("Check your phone connection and try again.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
    }
}

void DialogRecovery::closeEvent(QCloseEvent *event)
{
    // Let's decide if it's safe to exit or not
    if(busy)
    {
        event->ignore();
    }
    else
        event->accept();
}

void DialogRecovery::on_exploreButton_clicked()
{
    QDir temp_path(QCoreApplication::applicationDirPath());

#ifdef Q_OS_MACX
    // Because apple likes it's application folders
    temp_path.cdUp();
    temp_path.cdUp();
    temp_path.cdUp();
#endif

    QString path = QDir::toNativeSeparators(temp_path.absolutePath()+pathRecoveries);
    QDesktopServices::openUrl(QUrl("file:///" + path));
}

void DialogRecovery::on_tableWidget_itemClicked()
{
    ui->flashButton->setEnabled(true);
    ui->bootButton->setEnabled(true);
}

void DialogRecovery::on_actionRefresh_triggered()
{
    getFiles();
}

void DialogRecovery::on_actionDelete_triggered()
{
    if(ui->tableWidget->currentItem() != NULL)
    {
        // Prepare a messagebox
        QMessageBox msgBox(this->parentWidget());
        QPixmap icon(":/Icons/recovery.png");
        msgBox.setIconPixmap(icon);
        msgBox.setText("Are you sure you want to delete?");
        msgBox.setInformativeText("The file will be PERMANENTLY deleted from you hard drive.");
        msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        int option = msgBox.exec();

        if(option == QMessageBox::Yes)
        {
            QDir temp_path(QCoreApplication::applicationDirPath());

#ifdef Q_OS_MACX
            // Because apple likes it's application folders
            temp_path.cdUp();
            temp_path.cdUp();
            temp_path.cdUp();
#endif

            QFile file(temp_path.absolutePath()+pathRecoveries+ui->tableWidget->item(ui->tableWidget->currentRow() ,0)->text());
            file.remove();

            getFiles();
        }
    }
    else
    {
        // Prepare a messagebox
        QMessageBox msgBox(this->parentWidget());
        QPixmap icon(":/Icons/recovery.png");
        msgBox.setIconPixmap(icon);
        msgBox.setText("Nothing selected!");
        msgBox.setInformativeText("You need to select something in order to delete...");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
    }
}

void DialogRecovery::on_bootButton_clicked()
{
    if (DeviceConnection::getConnection(DEFAULT_TIMEOUT) == FASTBOOT)
    {
        if(ui->tableWidget->currentItem() != NULL)
        {
            QDir temp_path(QCoreApplication::applicationDirPath());

#ifdef Q_OS_MACX
            // Because apple likes it's application folders
            temp_path.cdUp();
            temp_path.cdUp();
            temp_path.cdUp();
#endif

            QProcess* process_boot = new QProcess(this);
            QString temp_cmd = QDir::toNativeSeparators(temp_path.absolutePath()+pathRecoveries)+ui->tableWidget->item(ui->tableWidget->currentRow() ,0)->text()+"\"\n";
            QString recovery;

            connect( process_boot, SIGNAL(finished(int)), this, SLOT(processFinished(int)));

            // Restrict from closing while booting
            busy = true;

            process_boot->setWorkingDirectory(QDir::toNativeSeparators(temp_path.absolutePath()+"/Data/"));
#ifdef Q_OS_WIN
            // Windows code here
            process_boot->start("cmd");
            recovery = "fastboot.exe-c \"lge.kcal=0|0|0|x\" boot \"" + temp_cmd;
#elif defined(Q_OS_MACX)
            // MAC code here
            process_boot->start("sh");
            recovery = "./fastboot_mac -c \"lge.kcal=0|0|0|x\" boot \"" + temp_cmd;
#else
            // Linux code here
            process_boot->start("sh");
            recovery = "./fastboot_linux -c \"lge.kcal=0|0|0|x\" boot \"" + temp_cmd;
#endif
            process_boot->waitForStarted();
            process_boot->write(recovery.toLatin1());
            process_boot->write("exit\n");

            // UI restrictions
            ui->tableWidget->setEnabled(false);
            ui->buttonBox->setEnabled(false);
            ui->flashButton->setEnabled(false);
            ui->progressBar->setVisible(true);
            ui->progressBar->setValue(0);
        }
    }
    else
    {
        // Prepare a messagebox
        QMessageBox msgBox(this->parentWidget());
        QPixmap icon(":/Icons/recovery.png");
        msgBox.setIconPixmap(icon);
        msgBox.setText("No connection!");
        msgBox.setInformativeText("Check your phone connection and try again.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
    }
}
