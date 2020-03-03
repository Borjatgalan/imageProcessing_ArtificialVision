#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    cap = new VideoCapture(0);
    winSelected = false;

    colorImage.create(240,320,CV_8UC3);
    grayImage.create(240,320,CV_8UC1);
    destColorImage.create(240,320,CV_8UC3);
    destGrayImage.create(240,320,CV_8UC1);
    //Auxiliares
    destColorImageAux.create(240,320,CV_8UC3);
    destGrayImageAux.create(240,320,CV_8UC1);

    visorHistoS = new ImgViewer(260,150, (QImage *) NULL, ui->histoFrameS);
    visorHistoD = new ImgViewer(260,150, (QImage *) NULL, ui->histoFrameD);


    visorS = new ImgViewer(&grayImage, ui->imageFrameS);
    visorD = new ImgViewer(&destGrayImage, ui->imageFrameD);


    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
    connect(ui->captureButton,SIGNAL(clicked(bool)),this,SLOT(start_stop_capture(bool)));
    connect(ui->colorButton,SIGNAL(clicked(bool)),this,SLOT(change_color_gray(bool)));
    connect(visorS,SIGNAL(windowSelected(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    connect(visorS,SIGNAL(pressEvent()),this,SLOT(deselectWindow()));
    //Connect Load and save from file
    connect(ui->loadButton,SIGNAL(pressed()),this,SLOT(loadFromFile()));
    connect(ui->saveButton,SIGNAL(pressed()),this,SLOT(saveToFile()));

    //TODO connections
    connect(ui->selectOperation,SIGNAL(activated(int)),this, SLOT(transformPixel()));
    connect(ui->selectOperation,SIGNAL(activated(int)),this, SLOT(thresholding()));
    connect(ui->selectOperation,SIGNAL(activated(int)),this, SLOT(equalize()));
    connect(ui->selectOperation,SIGNAL(activated(int)),this, SLOT(applyGaussianBlur()));
    connect(ui->selectOperation,SIGNAL(activated(int)),this, SLOT(applyMedianBlur()));
    connect(ui->selectOperation,SIGNAL(activated(int)),this, SLOT(linearFilter()));
    connect(ui->selectOperation,SIGNAL(activated(int)),this, SLOT(dilate()));
    connect(ui->selectOperation,SIGNAL(activated(int)),this, SLOT(erode()));
    connect(ui->selectOperation,SIGNAL(activated(int)),this, SLOT(applySeveral()));


    connect(ui->pixelTButton,SIGNAL(clicked()),this,SLOT(setPixelTransformation()));
    connect(ui->kernelButton,SIGNAL(clicked()),this,SLOT(setKernel()));
    connect(ui->operOrderButton,SIGNAL(clicked()),this,SLOT(setOperationOrder()));


    timer.start(30);


}

MainWindow::~MainWindow()
{
    delete ui;
    delete cap;
    delete visorS;
    delete visorD;
}

void MainWindow::compute()
{
    //Captura de imagen

    if(ui->captureButton->isChecked() && cap->isOpened())
    {
        *cap >> colorImage;

        cv::resize(colorImage, colorImage, Size(320, 240));
        cvtColor(colorImage, grayImage, COLOR_BGR2GRAY);
        cvtColor(colorImage, colorImage, COLOR_BGR2RGB);

    }


    //Procesamiento
    //QComboBox box;
    //box.currentIndex()

    //Actualización de los visores
    if(!ui->colorButton->isChecked())
    {
        selectOperation(grayImage, destGrayImage);
        updateHistograms(grayImage, visorHistoS);
        updateHistograms(destGrayImage, visorHistoD);
    }else{
        selectOperation(colorImage, destColorImage);
        updateHistograms(colorImage, visorHistoS);
        updateHistograms(destColorImage, visorHistoD);
    }

    if(winSelected)
    {
        visorS->drawSquare(QPointF(imageWindow.x+imageWindow.width/2, imageWindow.y+imageWindow.height/2), imageWindow.width,imageWindow.height, Qt::green );
    }
    visorS->update();
    visorD->update();
    visorHistoS->update();
    visorHistoD->update();

}

void MainWindow::updateHistograms(Mat image, ImgViewer * visor)
{
    if(image.type() != CV_8UC1) return;

    Mat histogram;
    int channels[] = {0,0};
    int histoSize = 256;
    float grange[] = {0, 256};
    const float * ranges[] = {grange};
    double minH, maxH;

    calcHist( &image, 1, channels, Mat(), histogram, 1, &histoSize, ranges, true, false );
    minMaxLoc(histogram, &minH, &maxH);

    float maxY = visor->getHeight();

    for(int i = 0; i<256; i++)
    {
        float hVal = histogram.at<float>(i);
        float minY = maxY-hVal*maxY/maxH;

        visor->drawLine(QLineF(i+2, minY, i+2, maxY), Qt::red);
    }

}


void MainWindow::start_stop_capture(bool start)
{
    if(start)
        ui->captureButton->setText("Stop capture");
    else
        ui->captureButton->setText("Start capture");
}

void MainWindow::change_color_gray(bool color)
{
    if(color)
    {
        ui->colorButton->setText("Gray image");
        visorS->setImage(&colorImage);
        visorD->setImage(&destColorImage);

    }
    else
    {
        ui->colorButton->setText("Color image");
        visorS->setImage(&grayImage);
        visorD->setImage(&destGrayImage);

    }
}

void MainWindow::selectWindow(QPointF p, int w, int h)
{
    QPointF pEnd;
    if(w>0 && h>0)
    {
        imageWindow.x = p.x()-w/2;
        if(imageWindow.x<0)
            imageWindow.x = 0;
        imageWindow.y = p.y()-h/2;
        if(imageWindow.y<0)
            imageWindow.y = 0;
        pEnd.setX(p.x()+w/2);
        if(pEnd.x()>=320)
            pEnd.setX(319);
        pEnd.setY(p.y()+h/2);
        if(pEnd.y()>=240)
            pEnd.setY(239);
        imageWindow.width = pEnd.x()-imageWindow.x;
        imageWindow.height = pEnd.y()-imageWindow.y;

        winSelected = true;
    }
}

void MainWindow::deselectWindow()
{
    winSelected = false;
}

void MainWindow::loadFromFile()
{
    disconnect(&timer,SIGNAL(timeout()),this,SLOT(compute()));

    Mat image;
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open"), "/home",tr("Images (*.jpg *.png "
                                                                                "*.jpeg *.gif);;All Files(*)"));
    image = cv::imread(fileName.toStdString());

    if (fileName.isEmpty())
        return;
    else {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::information(this, tr("Unable to open file"), file.errorString());
            return;
        }
        ui->captureButton->setChecked(false);
        ui->captureButton->setText("Start capture");
        cv::resize(image, colorImage, Size(320, 240));
        cvtColor(colorImage, colorImage, COLOR_BGR2RGB);
        cvtColor(colorImage, grayImage, COLOR_RGB2GRAY);

        connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));

    }
}

void MainWindow::saveToFile()
{
    disconnect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
    Mat save_image;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image File"),
                                                    QString(),
                                                    tr("JPG (*.jpg);; PNG (*.png);;"
                                                       "JPEG(*.jpeg);; GIF(*.gif);; All Files (*)"));
    if(ui->colorButton->isChecked())
        cvtColor(destColorImage, save_image, COLOR_RGB2BGR);

    else
        cvtColor(destGrayImage, save_image, COLOR_GRAY2BGR);

    if (fileName.isEmpty())
        return;
    else {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::information(this, tr("Unable to open file"),
                                     file.errorString());
            return;
        }
    }
    cv::imwrite(fileName.toStdString(), save_image);

    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
}

void MainWindow::transformPixel()
{
    printf("Transform Pixel... \n");

}

void MainWindow::thresholding(Mat image, Mat destImage)
{
    printf("Thresholding... \n");
    cv::threshold(image, destImage, ui->thresholdSpinBox->value(), 255, THRESH_BINARY);
    destImage.copyTo(destGrayImageAux);
}

void MainWindow::equalize()
{
    printf("Equalize... \n");
}

void MainWindow::applyGaussianBlur()
{
    printf("Gaussian Blur... \n");
}

void MainWindow::applyMedianBlur()
{
    printf("Median Blur... \n");
}

void MainWindow::linearFilter()
{
    printf("Linear Filter... \n");
}

void MainWindow::dilate()
{
    printf("Dilate... \n");
}

void MainWindow::erode()
{
    printf("Erode... \n");
}

void MainWindow::applySeveral()
{
    printf("Apply several... \n");
}




void MainWindow::setPixelTransformation()
{
    printf("set pixel trans\n");
}

void MainWindow::setKernel()
{
    printf("set kernel \n");
}

void MainWindow::setOperationOrder()
{
    printf("set operation\n");
}

void MainWindow::selectOperation(Mat image, Mat destImage)
{

    switch(ui->selectOperation->currentIndex()){
    case 0:
        transformPixel();
        break;
    case 1:
        thresholding(image, destImage);
        break;
    case 2:
        equalize();
        break;
    case 3:
        applyGaussianBlur();
        break;
    case 4:
        applyMedianBlur();
        break;
    case 5:
        linearFilter();
        break;
    case 6:
        dilate();
        break;
    case 7:
        erode();
        break;
    case 8:
        applySeveral();
        break;


    }
}



