#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->selectOperation->setCurrentIndex(-1);

    cap = new VideoCapture(0);
    winSelected = false;

    //Creación de las diferentes imágenes que vamos a utilizar
    colorImage.create(240,320,CV_8UC3);
    grayImage.create(240,320,CV_8UC1);
    destColorImage.create(240,320,CV_8UC3);
    destGrayImage.create(240,320,CV_8UC1);

    //Auxiliares
    destColorImageAux.create(240,320,CV_8UC3);
    destGrayImageAux.create(240,320,CV_8UC1);

    //Visores de los histogramas
    visorHistoS = new ImgViewer(260,150, (QImage *) NULL, ui->histoFrameS);
    visorHistoD = new ImgViewer(260,150, (QImage *) NULL, ui->histoFrameD);

    //Visores de las imagenes
    visorS = new ImgViewer(&grayImage, ui->imageFrameS);
    visorD = new ImgViewer(&destGrayImage, ui->imageFrameD);

    //Inicializacion de la matriz de kernels
    kernel.create(3,3,CV_32F);

    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
    connect(ui->captureButton,SIGNAL(clicked(bool)),this,SLOT(start_stop_capture(bool)));
    connect(ui->colorButton,SIGNAL(clicked(bool)),this,SLOT(change_color_gray(bool)));
    connect(visorS,SIGNAL(windowSelected(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    connect(visorS,SIGNAL(pressEvent()),this,SLOT(deselectWindow()));
    //Connect Load and save from file
    connect(ui->loadButton,SIGNAL(pressed()),this,SLOT(loadFromFile()));
    connect(ui->saveButton,SIGNAL(pressed()),this,SLOT(saveToFile()));

    //Botón de transformación de píxel
    connect(ui->pixelTButton,SIGNAL(clicked(bool)),this,SLOT(setPixelTransformation()));

    //Boton de kernel
    connect(ui->kernelButton,SIGNAL(clicked(bool)),this,SLOT(setKernel()));

    //Botón de orden de operaciones
    connect(ui->operOrderButton,SIGNAL(clicked(bool)),this,SLOT(setOperationOrder()));



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

    //Actualización de los visores
    if(!ui->colorButton->isChecked())
    {
        selectOperation(grayImage, destGrayImage, ui->selectOperation->currentIndex());
        updateHistograms(grayImage, visorHistoS);
        updateHistograms(destGrayImage, visorHistoD);
    }else{
        selectOperation(colorImage, destColorImage, ui->selectOperation->currentIndex());
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
                                                    tr("JPG (*.JPG) ; jpg (*.jpg); png (*.png); jpeg(*.jpeg); gif(*.gif); All Files (*)"));
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

void MainWindow::transformPixel(Mat image, Mat destImage)
{
    tablaLUT.resize(256);

    //Pixeles de origen en la matriz
    float orig1 = pixelTDialog.origPixelBox1->value();
    float orig2 = pixelTDialog.origPixelBox2->value();
    float orig3 = pixelTDialog.origPixelBox3->value();
    float orig4 = pixelTDialog.origPixelBox4->value();

    //Pixeles en la matriz nuevos
    float new1 = pixelTDialog.newPixelBox1->value();
    float new2 = pixelTDialog.newPixelBox2->value();
    float new3 = pixelTDialog.newPixelBox3->value();
    float new4 = pixelTDialog.newPixelBox4->value();

    float numAux = 0.;
    for (int i = orig1; i < orig2; i++) {
            numAux = (((i-orig1)*(new2-new1))/(orig2-orig1))+new1;
            tablaLUT[i] = numAux;
        }
    for (int i = orig2; i < orig3; i++) {
            numAux = (((i-orig2)*(new3-new2))/(orig3-orig2))+new2;
            tablaLUT[i] = numAux;
        }
    for (int i = orig3; i < orig4; i++) {
            numAux = (((i-orig3)*(new4-new3))/(orig4-orig3))+new3;
            tablaLUT[i] = numAux;
        }

    LUT(image, tablaLUT, destImage);
}

void MainWindow::thresholding(Mat image, Mat destImage)
{
    cv::threshold(image, destImage, ui->thresholdSpinBox->value(), 255, THRESH_BINARY);
    destImage.copyTo(destGrayImageAux);
}

void MainWindow::equalize(Mat image, Mat destImage)
{
    cv::equalizeHist(image, destImage);
    destImage.copyTo(destGrayImageAux);
}

void MainWindow::applyGaussianBlur(Mat image, Mat destImage)
{
    double gaussValue = ui->gaussWidthBox->value()/5.0;
    cv::GaussianBlur(image, destImage, Size(ui->gaussWidthBox->value(), ui->gaussWidthBox->value()),gaussValue);
    destGrayImage.copyTo(destGrayImageAux);

}

void MainWindow::applyMedianBlur(Mat image, Mat destImage)
{
    cv::medianBlur(image, destImage, 3);
    destGrayImage.copyTo(destGrayImageAux);
}

void MainWindow::setKernel()
{
    connect(ui->kernelButton, SIGNAL(clicked()), &lFilterDialog, SLOT(show()));
    connect(lFilterDialog.okButton, SIGNAL(clicked()), &lFilterDialog, SLOT(close()));

    printf("set kernel \n");
    kernel.at<float>(0,0)=lFilterDialog.kernelBox11->value();
    kernel.at<float>(0,1)=lFilterDialog.kernelBox12->value();
    kernel.at<float>(0,2)=lFilterDialog.kernelBox13->value();
    kernel.at<float>(1,0)=lFilterDialog.kernelBox21->value();
    kernel.at<float>(1,1)=lFilterDialog.kernelBox22->value();
    kernel.at<float>(1,2)=lFilterDialog.kernelBox23->value();
    kernel.at<float>(2,0)=lFilterDialog.kernelBox31->value();
    kernel.at<float>(2,1)=lFilterDialog.kernelBox32->value();
    kernel.at<float>(2,2)=lFilterDialog.kernelBox33->value();
}

void MainWindow::linearFilter(Mat image, Mat destImage)
{
    setKernel();
    cv::filter2D(image, destImage, CV_8U, kernel, Point(-1,-1), lFilterDialog.addedVBox->value());
}

void MainWindow::dilate(Mat image, Mat destImage)
{
    Mat aux;
    cv::threshold(image, destImage, ui->thresholdSpinBox->value(), 255, THRESH_BINARY);
    cv::dilate(destImage, destGrayImage, aux);
    destGrayImage.copyTo(destImage);
}

void MainWindow::erode(Mat image, Mat destImage)
{
    Mat aux;
    cv::threshold(image, destImage,ui->thresholdSpinBox->value(), 255, THRESH_BINARY);
    cv::erode(destImage,destGrayImage,aux);
    destGrayImage.copyTo(destImage);
}

void MainWindow::applySeveral(Mat image, Mat destImage)
{
    if(operOrderDialog.firstOperCheckBox->isChecked())
        selectOperation(image, destImage,operOrderDialog.operationComboBox1->currentIndex());

    if(operOrderDialog.secondOperCheckBox->isChecked())
        selectOperation(image, destImage,operOrderDialog.operationComboBox2->currentIndex());

    if(operOrderDialog.thirdOperCheckBox->isChecked())
        selectOperation(image, destImage,operOrderDialog.operationComboBox3->currentIndex());

    if(operOrderDialog.fourthOperCheckBox->isChecked())
        selectOperation(image, destImage,operOrderDialog.operationComboBox4->currentIndex());
}


void MainWindow::setPixelTransformation()
{
    connect(ui->pixelTButton,SIGNAL(clicked(bool)), &pixelTDialog, SLOT(show()));
    connect(pixelTDialog.okButton,SIGNAL(clicked(bool)),&pixelTDialog, SLOT(close()));
}


void MainWindow::setOperationOrder()
{
    connect(ui->operOrderButton,SIGNAL(clicked(bool)),&operOrderDialog, SLOT(show()));
    connect(operOrderDialog.okButton,SIGNAL(clicked(bool)), &operOrderDialog, SLOT(close()));
}


void MainWindow::selectOperation(Mat image, Mat destImage, int option)
{

    switch(option){
    case 0:
        transformPixel(image, destImage);
        break;
    case 1:
        thresholding(image, destImage);
        break;
    case 2:
        equalize(image, destImage);
        break;
    case 3:
        applyGaussianBlur(image, destImage);
        break;
    case 4:
        applyMedianBlur(image, destImage);
        break;
    case 5:
        linearFilter(image, destImage);
        break;
    case 6:
        dilate(image, destImage);
        break;
    case 7:
        erode(image, destImage);
        break;
    case 8:
        applySeveral(image, destImage);
        break;


    }
}



