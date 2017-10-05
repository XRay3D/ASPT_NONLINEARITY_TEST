#include "table.h"
#include <QAxObject>
#include <QApplication>
#include <QDateTime>

enum ColumnType {
    RowNum,
    SignalCh0,
    MeasureDeltaCh0,
    MeasurePpmCh0,
    MeasureCh0,
    SignalCh1,
    MeasureDeltaCh1,
    MeasurePpmCh1,
    MeasureCh1,
};

TABLE::TABLE(QWidget* parent)
    : QTableWidget(16, 9, parent)
    , dataChanged(false)
    , data(16)
    , average(16)
    , cellText(16)
    , resistors({ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 })
#ifdef EXCEL
    , excel(new Excel::Application(this))
#endif
{
    for (QVector<QVector<double> >& v : data) {
        v.resize(6);
        for (QVector<double>& d : v) {
            d.fill(0.0, 1);
        };
    }

    for (QVector<double>& v : average) {
        v.fill(0.0, 6);
    }

    for (QVector<QVector<QString> >& v : cellText) {
        v.resize(5);
        int i = 0;
        for (QVector<QString>& d : v) {
            switch (i++) {
            case RowNum:
                break;
            case SignalCh0:
                d.fill("", 6);
                break;
            case MeasureDeltaCh0:
                d.fill("", 6);
                break;
            case MeasurePpmCh0:
                d.fill("", 6);
                break;
            case MeasureCh0:
                d.fill("", 2);
                break;
            }
        };
    }

    QFont f(font());
    f.setPointSizeF(f.pointSizeF() * 3);

    setHorizontalHeaderLabels({ "№", "Сигнал, Ом", "∆, мОм", "PPM", "Канал АЦП 0", "Сигнал, Ом", "∆, мОм", "PPM", "Канал АЦП 1" });
    verticalHeader()->setVisible(false);

    for (int row = 0; row < rowCount(); ++row) {
        for (int col = 0; col < columnCount(); ++col) {
            QTableWidgetItem* tableitem = new QTableWidgetItem("QTableWidgetItem");
            setItem(row, col, tableitem);

            switch (col) {
            case RowNum:
                tableitem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
                tableitem->setCheckState(Qt::Checked);
                tableitem->setTextAlignment(Qt::AlignCenter);
                tableitem->setText(QString("%1").arg(row + 1));
                break;
            case SignalCh0:
            case MeasureDeltaCh0:
            case MeasurePpmCh0:
            case SignalCh1:
            case MeasureDeltaCh1:
            case MeasurePpmCh1:
                tableitem->setFlags(Qt::ItemIsEnabled);
                break;
            case MeasureCh0:
            case MeasureCh1:
                tableitem->setFlags(Qt::ItemIsEnabled);
                tableitem->setFont(f);
                break;
            }
        }
        Update(row);
    }
    dataChanged = true;
    resizeEvent(0);

    setColumnHidden(MeasureDeltaCh0, true);
    setColumnHidden(MeasurePpmCh0, true);
    setColumnHidden(MeasureDeltaCh1, true);
    setColumnHidden(MeasurePpmCh1, true);

    connect(this, &QTableWidget::itemClicked, [&](QTableWidgetItem* Item) {
        if (Item->column() == RowNum) {
            for (int col = SignalCh0; col < columnCount(); ++col) {
                item(Item->row(), col)->setFlags(Item->checkState() == Qt::Checked ? Qt::ItemIsEnabled : Qt::NoItemFlags);
            }
        }
    });

    setIconSize(QSize(24, 24));

#ifdef EXCEL
    excel->SetVisible(true);
#endif
}

TABLE::~TABLE()
{
#ifdef EXCEL
    excel->Quit();
#endif
}

void TABLE::LoadFile(const QString& fileName)
{
    qDebug() << "LoadFile" << fileName;
#ifdef EXCEL
    if (!excel->isNull()) {
        Excel::_Workbook pWb(0, excel->Workbooks()->querySubObject("Open(const QString&)", fileName) /*->Open(fileName)*/);
        if (excel->Workbooks()->Count()) {
            for (int devCh = 0; devCh < 16; ++devCh) {
                for (int adcCh = 0; adcCh < 2; ++adcCh) {
                    for (int resCh = 0; resCh < 3; ++resCh) {
                        data[devCh][adcCh * 3 + resCh].clear();
                        if (!adcCh)
                            data[devCh][adcCh * 3 + resCh].append(excel->Range(QString("D%1").arg(6 + devCh * 3 + resCh))->Value().toDouble());
                        else
                            data[devCh][adcCh * 3 + resCh].append(excel->Range(QString("G%1").arg(6 + devCh * 3 + resCh))->Value().toDouble());
                    }
                }
                Update(devCh);
            }
            pWb.Close();
            //excel->Workbooks()->Close();
            dataChanged = true;
            resizeEvent(0);
        }
    }
#else
    curFile = fileName;
    QAxObject* excel = new QAxObject("Excel.Application", 0);
    excel->dynamicCall("SetVisible(bool)", true);
    QAxObject* workbooks = excel->querySubObject("Workbooks");
    QAxObject* workbook = workbooks->querySubObject("Open(const QString&)", curFile);
    QAxObject* sheets = workbook->querySubObject("Worksheets");
    QVariant value;
    if (workbooks->dynamicCall("Count()").toInt()) {
        QAxObject* sheet = sheets->querySubObject("Item(int)", 1);
        for (int devCh = 0; devCh < 16; ++devCh) {
            for (int adcCh = 0; adcCh < 2; ++adcCh) {
                for (int resCh = 0; resCh < 3; ++resCh) {
                    data[devCh][adcCh * 3 + resCh].clear();
                    if (!adcCh) {
                        QAxObject* cell = sheet->querySubObject("Cells(int,int)", 6 + devCh * 3 + resCh, 4);
                        value = cell->dynamicCall("Value()");
                    }
                    else {
                        QAxObject* cell = sheet->querySubObject("Cells(int,int)", 6 + devCh * 3 + resCh, 7);
                        value = cell->dynamicCall("Value()");
                    }
                    data[devCh][adcCh * 3 + resCh].append(value.toDouble());
                }
            }
            Update(devCh);
        }
        workbook->dynamicCall("Close()");
        dataChanged = true;
        resizeEvent(0);
    }
    excel->dynamicCall("Quit()");
    excel->deleteLater();
#endif
}

void TABLE::SaveFile(const QString& fileName, const QString& asptNum, const QString& fio)
{
    qDebug() << "SaveFile" << fileName << asptNum << fio;
#ifdef EXCEL
    if (!excel->isNull()) {
        if (curFile.isEmpty()) {
            excel->Workbooks()->Open(qApp->applicationDirPath() + "/XX-XX от XX.XX.XX г.xlsx");
            excel->Range("D55")->SetValue(QDateTime::currentDateTime());
        }
        else {
            excel->Workbooks()->Open(curFile);
        }
        excel->Range("E2")->SetValue(asptNum);
        excel->Range("D57")->SetValue(fio);
        excel->Range("D56")->SetValue(QDateTime::currentDateTime());
        for (int devCh = 0; devCh < 16; ++devCh) {
            for (int adcCh = 0; adcCh < 2; ++adcCh) {
                for (int resCh = 0; resCh < 3; ++resCh) {
                    int num;
                    double average;
                    num = devCh * 6 + adcCh * 3 + resCh;
                    average = 0.0;
                    foreach (double val, data[num]) {
                        average += val;
                    }
                    average /= data[num].count();
                    if (!adcCh) {
                        excel->Range(QString("D%1").arg(6 + devCh * 3 + resCh))->SetValue(average);
                    }
                    else {
                        excel->Range(QString("G%1").arg(6 + devCh * 3 + resCh))->SetValue(average);
                    }
                }
            }
        }
        QString str = fileName;
        QTextCodec::setCodecForLocale(QTextCodec::codecForName("CP-1251"));
        if (curFile.isEmpty() || QString(*curFile) != fileName) {
            //            qDebug("if");
            excel->ActiveWorkbook()->SaveAs(str.replace('/', '\\'));
            //excel->ActiveWorkbook()->SaveAs(str.replace('/', '\\'),Excel::xlOpenXMLWorkbook,"","",);        }
        }
        else {
            //            qDebug("else");
            excel->ActiveWorkbook()->Save();
        }
        while (excel->Workbooks()->Count() > 0) {
            //            qDebug() << excel->ActiveWorkbook()->Name();
            excel->ActiveWorkbook()->Close(false);
        }
    }
#else
    bool newFile = false;
    QAxObject* excel = new QAxObject("Excel.Application", 0);
    excel->dynamicCall("SetVisible(bool)", true);
    QAxObject* workbooks = excel->querySubObject("Workbooks");
    if (curFile.isEmpty()) {
        curFile = qApp->applicationDirPath() + "/XX-XX от XX.XX.XX г.xlsx";
        newFile = true;
    }
    QAxObject* workbook = workbooks->querySubObject("Open(const QString&)", curFile);
    QAxObject* sheets = workbook->querySubObject("Worksheets");
    if (workbooks->dynamicCall("Count()").toInt()) {
        QAxObject* sheet = sheets->querySubObject("Item(int)", 1);
        QAxObject* cell;
        if (newFile) {
            cell = sheet->querySubObject("Cells(int,int)", 55, 4);
            cell->dynamicCall("SetValue2(QVariant)", QDateTime::currentDateTime());
        }
        cell = sheet->querySubObject("Cells(int,int)", 2, 5);
        cell->dynamicCall("SetValue2(QVariant)", asptNum);
        cell = sheet->querySubObject("Cells(int,int)", 57, 4);
        cell->dynamicCall("SetValue2(QVariant)", fio);
        cell = sheet->querySubObject("Cells(int,int)", 56, 4);
        cell->dynamicCall("SetValue2(QVariant)", QDateTime::currentDateTime());
        double v = 0.0;
        for (int devCh = 0; devCh < 16; ++devCh) {
            for (int adcCh = 0; adcCh < 2; ++adcCh) {
                for (int resCh = 0; resCh < 3; ++resCh) {
                    QAxObject* cell;
                    if (!adcCh) {
                        cell = sheet->querySubObject("Cells(int,int)", 6 + devCh * 3 + resCh, 4);
                    }
                    else {
                        cell = sheet->querySubObject("Cells(int,int)", 6 + devCh * 3 + resCh, 7);
                    }
                    cell->dynamicCall("SetValue2(QVariant)", average[devCh][adcCh * 3 + resCh]);
                }
            }
            Update(devCh);
        }
        if (newFile) {
            workbook->dynamicCall("SaveAs(const QVariant&)", QVariant(QString(fileName).replace('/', '\\')));
        }
        else {
            workbook->dynamicCall("Save()");
        }
        workbook->dynamicCall("Close()");
    }
    excel->dynamicCall("Quit()");
    excel->deleteLater();
#endif
}

void TABLE::PrintFile(const QString& fileName)
{
    qDebug() << "PrintFile" << fileName;
#ifdef EXCEL
    if (!excel->isNull()) {
        if (excel->Workbooks()->Open(fileName)) {
            Excel::_Worksheet sheet(excel->ActiveSheet());
            sheet.PrintOut(1, 1, 1);
            excel->Workbooks()->Close(); //ActiveWorkbook()->Close(false);
        }
    }
#else
    QAxObject* excel = new QAxObject("Excel.Application", 0);
    excel->dynamicCall("SetVisible(bool)", true);
    QAxObject* workbooks = excel->querySubObject("Workbooks");
    QAxObject* workbook = workbooks->querySubObject("Open(const QString&)", fileName);
    QAxObject* sheets = workbook->querySubObject("Worksheets");
    if (workbooks->dynamicCall("Count()").toInt()) {
        QAxObject* sheet = sheets->querySubObject("Item(int)", 1);
        sheet->dynamicCall("PrintOut(QVariant, QVariant, QVariant)", 1, 1, 1);
        workbook->dynamicCall("Close()");
    }
    excel->dynamicCall("Quit()");
    excel->deleteLater();
#endif
}

void TABLE::Update(int row)
{
    for (int i = 0; i < 6; ++i) {
        Update(row, i);
    }
}

void TABLE::Update(int row, int pos)
{
    double delta;
    double min_ = 0.0;
    double max_ = 0.0;

    if (data[row][pos].count()) {
        QVector<double> v(data[row][pos]);
        qSort(v);
        min_ = v.first();
        max_ = v.last();
        average[row][pos] = 0;
        if (v.count() > skip) {
            for (int i = ceil(skip * 0.5); i < (data[row][pos].count() - floor(skip * 0.5)); ++i) {
                average[row][pos] += v[i];
            }
            average[row][pos] /= v.count() - skip;
        }
        else {
            for (double val : v) {
                average[row][pos] += val;
            }
            average[row][pos] /= v.count();
        }
    }

    int i = (pos % 3);

    cellText[row][SignalCh0][pos] = QString("R%1 = %2%3")
                                        .arg(i + 1)
                                        .arg(average[row][pos], 0, 'f', 5)
                                        .arg(i < 2 ? "\n" : "")
                                        .replace('.', ',');

    cellText[row][MeasureDeltaCh0][pos] = QString("%1%2")
                                              .arg((max_ - min_) * 1000.0, 0, 'g', 3)
                                              .arg(i < 2 ? "\n" : "")
                                              .replace('.', ',');

    cellText[row][MeasurePpmCh0][pos] = QString("%1%2")
                                            .arg(resistors[i] > 0.0 ? ((average[row][pos] / resistors[i]) - 1.0) * 1.e6 : 0.0, 0, 'g', 4)
                                            .arg(i < 2 ? "\n" : "");

    QColor color;
    if (pos < 3) {
        delta = average[row][0] + average[row][1] - average[row][2];
        cellText[row][MeasureCh0][0] = QString("%1%2").arg(delta < 0 ? "" : "").arg(delta, 0, 'f', 6).replace('.', ',');
    }
    else {
        delta = average[row][3] + average[row][4] - average[row][5];
        cellText[row][MeasureCh0][1] = QString("%1%2").arg(delta < 0 ? "" : "").arg(delta, 0, 'f', 6).replace('.', ',');
    }

    delta = abs(delta);
    QIcon icon;
    if (delta > max) {
        color = QColor::fromHsv(0, 50, 255);
        icon = QIcon("icon1.svg");
    }
    else if (delta < min) {
        color = QColor::fromHsv(120, 50, 255);
        icon = QIcon("icon2.svg");
    }
    else {
        color = QColor::fromHsv(120 - ((120 / (max - min)) * (delta - min)), 50, 255);
        icon = QIcon("icon3.svg");
    }

    if (pos < 3) {
        item(row, SignalCh0)->setText(cellText[row][SignalCh0][0] + cellText[row][SignalCh0][1] + cellText[row][SignalCh0][2]);
        item(row, MeasureDeltaCh0)->setText(cellText[row][MeasureDeltaCh0][0] + cellText[row][MeasureDeltaCh0][1] + cellText[row][MeasureDeltaCh0][2]);
        item(row, MeasurePpmCh0)->setText(cellText[row][MeasurePpmCh0][0] + cellText[row][MeasurePpmCh0][1] + cellText[row][MeasurePpmCh0][2]);
        item(row, MeasureCh0)->setText(cellText[row][MeasureCh0][0]);
        item(row, MeasureCh0)->setBackgroundColor(color);
        //item(row, MeasureCh0)->setForeground(Qt::black);
        item(row, MeasureCh0)->setIcon(icon);
    }
    else {
        item(row, SignalCh1)->setText(cellText[row][SignalCh0][3] + cellText[row][SignalCh0][4] + cellText[row][SignalCh0][5]);
        item(row, MeasureDeltaCh1)->setText(cellText[row][MeasureDeltaCh0][3] + cellText[row][MeasureDeltaCh0][4] + cellText[row][MeasureDeltaCh0][5]);
        item(row, MeasurePpmCh1)->setText(cellText[row][MeasurePpmCh0][3] + cellText[row][MeasurePpmCh0][4] + cellText[row][MeasurePpmCh0][5]);
        item(row, MeasureCh1)->setText(cellText[row][MeasureCh0][1]);
        item(row, MeasureCh1)->setBackgroundColor(color);
        //item(row, MeasureCh1)->setForeground(Qt::black);
        item(row, MeasureCh1)->setIcon(icon);
    }
}

void TABLE::CheckUncheckAll(bool checked)
{
    for (int row = 0; row < 16; ++row) {
        item(row, 0)->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
        for (int col = 1; col < columnCount(); ++col) {
            item(row, col)->setFlags(checked ? Qt::ItemIsEnabled : Qt::NoItemFlags);
        }
    }
}

void TABLE::CheckPpm(bool checked)
{
    setColumnHidden(MeasurePpmCh0, !checked);
    setColumnHidden(MeasurePpmCh1, !checked);
    dataChanged = true;
    resizeEvent(0);
}

void TABLE::CheckDelta(bool checked)
{
    setColumnHidden(MeasureDeltaCh0, !checked);
    setColumnHidden(MeasureDeltaCh1, !checked);
    dataChanged = true;
    resizeEvent(0);
}

void TABLE::clearSelectedData()
{
    QList<int> rows;
    for (int row = 0; row < 16; ++row) {
        if (item(row, 0)->checkState() == Qt::Checked)
            rows.append(row);
    }
    if (rows.size()) {
        if (QMessageBox::question(this, "", "Вы действительно хотите отчистить выделенные каналы?", "Да", "Нет", "", 1, 1) == 0) {
            for (int row : rows) {
                for (QVector<double>& v : data[row]) {
                    v.fill(0.0, 1);
                }
                Update(row);
            }
            dataChanged = true;
            resizeEvent(0);
        }
    }
}

void TABLE::clearData(int row)
{
    if (0 > row || row > 15)
        return;
    for (QVector<double>& v : data[row]) {
        v.clear();
    }
}

void TABLE::setResistors(const QVector<double>&& value)
{
    resistors = value;
}

void TABLE::resizeEvent(QResizeEvent* event)
{
    if (event != nullptr)
        event->accept();
    //    static QSize Size;
    //    if (Size != size() || dataChanged) {
    if (dataChanged) {
        resizeColumnsToContents();
        resizeRowsToContents();
    }
    int ColumnWidth = (size().width() - 2) - (columnWidth(RowNum) + columnWidth(SignalCh0) + columnWidth(SignalCh1));
    if (!isColumnHidden(MeasureDeltaCh0))
        ColumnWidth -= (columnWidth(MeasureDeltaCh0) + columnWidth(MeasureDeltaCh1));
    if (!isColumnHidden(MeasurePpmCh0))
        ColumnWidth -= (columnWidth(MeasurePpmCh0) + columnWidth(MeasurePpmCh1));
    if (verticalScrollBar()->isVisible())
        ColumnWidth -= verticalScrollBar()->width();
    setColumnWidth(MeasureCh0, ColumnWidth / 2.0);
    setColumnWidth(MeasureCh1, ColumnWidth / 2.0);
    dataChanged = false;
    //    Size = size();
    //    }
    update();
}

void TABLE::setCurFile(const QString& value)
{
    curFile = value;
}

void TABLE::setEnableRow(bool fl)
{
    if (fl)
        for (int row = 0; row < 16; ++row) {
            item(row, 0)->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        }
    else
        for (int row = 0; row < 16; ++row) {
            item(row, 0)->setFlags(Qt::ItemIsEnabled);
        }
}

QVector<double> TABLE::getData(int row, int pos) const
{
    return data[row][pos];
}

void TABLE::addData(int row, int pos, double val)
{
    data[row][pos].append(val);
    Update(row, pos);
    dataChanged = true;
    resizeEvent(0);
    emit updatePlot(row);
}

void TABLE::setSkip(int value)
{
    skip = value;
}

void TABLE::setMax(double value)
{
    max = value;
    dataChanged = true;
}

void TABLE::setMin(double value)
{
    min = value;
    dataChanged = true;
}

void TABLE::showEvent(QShowEvent* event)
{
    event->accept();
    if (dataChanged)
        for (int row = 0; row < rowCount(); ++row) {
            Update(row);
        }
    resizeEvent(0);
}
