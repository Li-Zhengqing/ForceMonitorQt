#include "ForceMonitorQt.h"

void UpdateData(ForceMonitorQt* app);

void EmitUpdateSignal(ForceMonitorQt* app);

/*
void ResetCh1(ForceMonitorQt* app);
void ResetCh2(ForceMonitorQt* app);
void ResetCh3(ForceMonitorQt* app);
*/

ForceMonitorQt::ForceMonitorQt(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    this->setWindowTitle(tr("ForceMonitor"));
    this->status_vis_label = new QLabel("Initializing...");
    this->statusBar()->addWidget(this->status_vis_label);

    this->temp1 = new PLC_BUFFER_TYPE[CLIENT_BUFFER_SIZE];
    this->temp2 = new PLC_BUFFER_TYPE[CLIENT_BUFFER_SIZE];
    this->temp3 = new PLC_BUFFER_TYPE[CLIENT_BUFFER_SIZE];
    
    this->temp[0] = this->temp1;
    this->temp[1] = this->temp2;
    this->temp[2] = this->temp3;

    // this->plc = new Plc();
    this->plc = new VirtualPlc();

    this->status = IPCStatus::Offline;

    this->data_file = "";
    // this->data_path = ".";
    // this->data_path = "../data";
    QDir _work_directory = QDir(QDir::currentPath());
    _work_directory.cd(tr("../data"));
    this->data_path = _work_directory.absolutePath().toStdString();
    
    this->recorder = NULL;

    this->data[0] = new QLineSeries();
    this->data[1] = new QLineSeries();
    this->data[2] = new QLineSeries();

    this->chart[0] = new QChart();
    this->chart[1] = new QChart();
    this->chart[2] = new QChart();

    this->regular_expression_validator = new QRegExpValidator(QRegExp("^\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}$"));
    // this->ui.lineEdit->setValidator(&QRegExpValidator(this->regular_expression_validator));

    connect(ui.pushButton, &QPushButton::clicked, this, &ForceMonitorQt::Connect);
    connect(ui.pushButton_2, &QPushButton::clicked, this, &ForceMonitorQt::Disconnect);
    connect(ui.pushButton_3, &QPushButton::clicked, this, &ForceMonitorQt::Start);
    connect(ui.pushButton_4, &QPushButton::clicked, this, &ForceMonitorQt::Stop);
    connect(ui.pushButton_5, &QPushButton::clicked, this, &ForceMonitorQt::SetWorkDirectory);

    // FIXME: Not good practice!
    connect(this, &ForceMonitorQt::updateUi, this, &ForceMonitorQt::updateData);

    connect(ui.menuMenu->actions()[0], &QAction::triggered, this, &ForceMonitorQt::Connect);
    connect(ui.menuMenu->actions()[1], &QAction::triggered, this, &ForceMonitorQt::Disconnect);
    connect(ui.menuMenu->actions()[2], &QAction::triggered, this, &ForceMonitorQt::Start);
    connect(ui.menuMenu->actions()[3], &QAction::triggered, this, &ForceMonitorQt::Stop);
    connect(ui.menuMenu->actions()[4], &QAction::triggered, this, &ForceMonitorQt::SetWorkDirectory);
    connect(ui.menuAbout->actions()[0], &QAction::triggered, this, &ForceMonitorQt::AboutPage);

    this->status_vis_label->setText("No PLC connected.");

    /*
    LOG(INFO) << "temp[0] address: " << this->temp[0] << std::endl;
    LOG(INFO) << "temp[1] address: " << this->temp[1] << std::endl;
    LOG(INFO) << "temp[2] address: " << this->temp[2] << std::endl;
    LOG(INFO) << "QLineSeries: data[0] address: " << this->data[0] << std::endl;
    LOG(INFO) << "QLineSeries: data[1] address: " << this->data[1] << std::endl;
    LOG(INFO) << "QLineSeries: data[2] address: " << this->data[2] << std::endl;
    */

    this->ui.comboBox->addItem(tr("EL3702"));
    this->ui.comboBox->addItem(tr("EL3356"));

    this->selected_variable_grp_id = this->ui.comboBox->currentIndex();

    this->ui.lineEdit_3->setText("0");
    this->ui.lineEdit_4->setText("0");
    this->ui.lineEdit_5->setText("0");
    for (int _channel = 0; _channel < 3; _channel++) {
        this->data_offset_reset_flag[_channel] = 0;
        this->data_offset_ref[_channel] = 0;
    }

    connect(ui.pushButton_6, &QPushButton::clicked, this, &ForceMonitorQt::resetCh1Offset);
    connect(ui.pushButton_7, &QPushButton::clicked, this, &ForceMonitorQt::resetCh2Offset);
    connect(ui.pushButton_8, &QPushButton::clicked, this, &ForceMonitorQt::resetCh3Offset);
}

ForceMonitorQt::~ForceMonitorQt()
{
    this->Disconnect();

    delete[] this->temp1;
    delete[] this->temp2;
    delete[] this->temp3;
}

void ForceMonitorQt::Start() {
    if (this->status == IPCStatus::Ready) {
        // Handle with logger
        this->getDataFileName();

        // Check if file exists
        // string _check_name = this->data_path + "/" + this->data_file;
        if (checkFile(this->data_path, this->data_file)) {
            QMessageBox::StandardButton _choice = QMessageBox::warning(this, 
                tr("File Existed."), tr("File exists, are you sure to overwrite the existing file?"), 
                QMessageBox::Ok | QMessageBox::Cancel);

            if (_choice == QMessageBox::Cancel) {
				// Do nothing and return.
				return;
            }
        }

        if (this->recorder) delete this->recorder;
        this->recorder = new DataLogger(this->data_path, this->data_file);

        // FIXME: avoiding if the plc is running
        this->plc->stopPlc();

        // Start PLC
		this->status = IPCStatus::Runing;

		for (int _channel = 0; _channel < 3; _channel++) {
			this->time_stamp[_channel] = 0;
		}

        this->selected_variable_grp_id = this->ui.comboBox->currentIndex();
        this->plc->setVariableGroupId(this->selected_variable_grp_id);
		this->plc->startPlc();
		// this->timer.start(250, std::bind(UpdateData, this));
		this->timer.start(500, std::bind(EmitUpdateSignal, this));
		this->initData();

		this->status_vis_label->setText("Running...");

        this->ui.comboBox->setDisabled(true);
    }
    else if (this->status == IPCStatus::Offline) {
        QMessageBox::critical(this, tr("Not Connected"), tr("No PLC devices is connected, please connect with a PLC first!"), QMessageBox::Ok);
    }
}

void ForceMonitorQt::Stop() {
    if (this->status == IPCStatus::Runing) {
		this->status = IPCStatus::Ready;

		this->plc->stopPlc();
		this->timer.stop();

        if (this->recorder) {
            // delete this->recorder;
        }

		this->status_vis_label->setText("Connected with PLC, Ready");
        this->InfoWorkDirectory();

        this->ui.comboBox->setEnabled(true);
    }
}

void ForceMonitorQt::Connect() {
    if (this->status == IPCStatus::Offline) {
        QString _q_address = this->ui.lineEdit->text();
        int _regex_i = 0;
        
        LOG(INFO) << "QValidator: " << this->regular_expression_validator->validate(_q_address, _regex_i);

        try {
            QStringList _address = _q_address.split(QString("."));

            if ((this->regular_expression_validator->validate(_q_address, _regex_i) == QValidator::State::Acceptable) && (_address.length() == 4)) {
                unsigned char target_address[4] = { 0, 0, 0, 0 };
                for (int i = 0; i < 4; i++) {
                    target_address[i] = static_cast<unsigned char>(_address[i].toInt());
                }
                // LOG(INFO) << target_address[0] << "." << target_address[1] << "." << target_address[2] << "." << target_address[3];

                LOG(INFO) << "Connecting with device: "
                    << static_cast<unsigned int>(target_address[0]) << "."
                    << static_cast<unsigned int>(target_address[1]) << "."
                    << static_cast<unsigned int>(target_address[2]) << "."
                    << static_cast<unsigned int>(target_address[3]);

                this->setDisabled(true);
                long _ret = this->plc->connectPlc(target_address);
                this->setDisabled(false);

                // long _ret = this->plc->connectLocalPlc();
                if (_ret) {
                    // TODO
                    QMessageBox::critical(this, tr("Connection Error"), tr("Failed to connect with PLC!"), QMessageBox::Ok);
                    return;
                }
                this->status = IPCStatus::Ready;
				this->ui.lineEdit->setDisabled(true);
            }
            else {
                if ((_q_address.length() != 0) && (QMessageBox::warning(
                    this, tr("Invalid Target Address"), tr("Invalid Target Address Input, connecting with local device"), 
                    QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel
                    )) {
                    return;
                }
                
                LOG(INFO) << "Connecting with local device";
                long _ret = this->plc->connectLocalPlc();
                if (_ret) {
                    // TODO
                    QMessageBox::critical(this, tr("Connection Error"), tr("Failed to connect with PLC!"), QMessageBox::Ok);
                    return;
                }
				this->ui.lineEdit->clear();
                this->status = IPCStatus::Ready;
				this->ui.lineEdit->setDisabled(true);
            }
        }
        catch (exception& e) {
            // LOG(WARNING) << "Failed to parse target address: " << _q_address.toStdString();
            LOG(WARNING) << "Failed to parse target address";
            LOG(INFO) << "Connecting with local device";

			if ((_q_address.length() != 0) && (QMessageBox::warning(
				this, tr("Fail to Establish Connection"), 
				tr("Failed to connect target device, do you want to try connect with local device?"), 
				QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel
				)) {
				return;
			}

            long _ret = this->plc->connectLocalPlc();
            if (_ret) {
                // TODO
                QMessageBox::critical(this, tr("Connection Error"), tr("Failed to connect with PLC!"), QMessageBox::Ok);
                return;
            }
            this->status = IPCStatus::Ready;
            this->ui.lineEdit->clear();
            this->ui.lineEdit->setDisabled(true);
        }
        // string _address = _q_address.toStdString();

		this->status_vis_label->setText("Connected with PLC, Ready");
    }
}

void ForceMonitorQt::Disconnect() {
    if (this->status == IPCStatus::Runing) {
        this->Stop();
		QMessageBox::warning(this, tr("Recording stopped"), tr("Recording is forced stopped by disconnecting operation"), QMessageBox::Ok);
    }

    if (this->status == IPCStatus::Ready) {
		this->status = IPCStatus::Offline;
		this->plc->disconnectPlc();
        this->ui.lineEdit->setDisabled(false);

		this->status_vis_label->setText("Not Connected with PLC");
    }
}

void ForceMonitorQt::initData() {
    unsigned int _length[3];
    this->plc->copyDataToClient(this->temp, _length);

    for (int _channel = 0; _channel < 3; _channel++) {
        this->chart[_channel]->close();

        this->data[_channel]->clear();
        this->data[_channel]->setUseOpenGL(true);
        // for (int i = 0; i < _length[_channel]; i += 10) {
        for (int i = 0; i < _length[_channel]; i += PLOT_STEP) {
			this->data[_channel]->clear();

            std::cout << "_length[" << _channel << "] = " << _length[_channel] << std::endl;
            this->time_stamp[_channel] += PLOT_STEP;
            this->data[_channel]->append(double(time_stamp[_channel]) / SAMPLE_RATE, this->temp[_channel][i]);
        }

        // FIXME: Memory leak here
        // Free memory of QChart
        // QChart* old_chart = this->chart[_channel];
        // this->chart[_channel]->removeAllSeries();
        this->chart[_channel] = new QChart();
        // delete old_chart;

        this->chart[_channel]->addSeries(this->data[_channel]);
        this->chart[_channel]->createDefaultAxes();
        this->chart[_channel]->axisX()->setMax(double(this->time_stamp[_channel]) / (SAMPLE_RATE));
        this->chart[_channel]->axisY()->setMax(4000);
        this->chart[_channel]->legend()->hide();

        this->chart[_channel]->axisX()->setTitleText(tr("t[s]"));
        this->chart[_channel]->axisY()->setTitleText(tr("F[N]"));
		// this->chart[_channel]->update();
    }

    this->ui.graphicsView->setChart(this->chart[0]);
    this->ui.graphicsView->setRenderHint(QPainter::Antialiasing);
    this->ui.graphicsView_2->setChart(this->chart[1]);
    this->ui.graphicsView_2->setRenderHint(QPainter::Antialiasing);
    this->ui.graphicsView_3->setChart(this->chart[2]);
    this->ui.graphicsView_3->setRenderHint(QPainter::Antialiasing);
    
}

void ForceMonitorQt::updateData() {
	/* Reset offset */
    for (int _channel = 0; _channel < 3; _channel++) {
		if (this->data_offset_reset_flag[_channel]) {
			PLC_BUFFER_TYPE ref_value = 0;
            bool _is_input_valid = false;
			try {
				switch (_channel)
				{
				case 0:
					ref_value = this->ui.lineEdit_3->text().toFloat(&_is_input_valid);
					break;
				case 1:
					ref_value = this->ui.lineEdit_4->text().toFloat(&_is_input_valid);
					break;
				case 2:
					ref_value = this->ui.lineEdit_5->text().toFloat(&_is_input_valid);
					break;
				default:
					break;
				}

                if (_is_input_valid) {
					this->data_offset_ref[_channel] = ref_value;
                }
                else {
					this->data_offset_reset_flag[_channel] = 0;
					switch (_channel)
					{
					case 0:
						this->ui.lineEdit_3->setText(tr("0"));
						break;
					case 1:
						this->ui.lineEdit_4->setText(tr("0"));
						break;
					case 2:
						this->ui.lineEdit_5->setText(tr("0"));
						break;
					default:
						break;
					}
                }
				// this->data_offset_reset_flag[_channel] = 0;
			}
			catch (exception e) {
				// Do nothing
				this->data_offset_reset_flag[_channel] = 0;
				switch (_channel)
				{
				case 0:
					this->ui.lineEdit_3->setText(tr("0"));
					break;
				case 1:
					this->ui.lineEdit_4->setText(tr("0"));
					break;
				case 2:
					this->ui.lineEdit_5->setText(tr("0"));
					break;
				default:
					break;
				}
			}
		}
    }
    this->plc->resetDataOffset(this->data_offset_reset_flag, this->data_offset_ref);
    for (int _channel = 0; _channel < 3; _channel++) {
		this->data_offset_reset_flag[_channel] = 0;
    }

    unsigned int _length[3];
    this->plc->copyDataToClient(this->temp, _length);

    this->logData(this->temp, _length);

    for (int _channel = 0; _channel < 3; _channel++) {
		LOG(INFO) << "_length[" << _channel << "] = " << _length[_channel] << std::endl;
        if (_length[_channel] == 0) {
            continue;
        }

        for (int i = 0; i < _length[_channel]; i += PLOT_STEP) {
            // FIXME: Bug seems to be here.
            // std::cout << "_length[" << _channel << "] = " << _length[_channel] << std::endl;
            try {
				this->time_stamp[_channel] += PLOT_STEP;
				this->data[_channel]->append(double(time_stamp[_channel]) / SAMPLE_RATE, this->temp[_channel][i]);
            }
            catch (exception e) {
                LOG(WARNING) << "Exception when adding data: " << _channel << "-" << i << std::endl;
            }

            // this->data[_channel]->remove(0);
        }
        // LOG(INFO) << "here begin" << std::endl;

        double _current_time = double(this->time_stamp[_channel]) / (SAMPLE_RATE);

        // Remove data out of date
        // LOG(INFO) << "here0" << std::endl;
        int _rm_len = this->data[_channel]->count() - (PLOT_DURATION * SAMPLE_RATE / PLOT_STEP);
        if (_rm_len > 0) {
			this->data[_channel]->removePoints(0, _rm_len);
        }
        /*
        while (this->data[_channel]->count() > PLOT_DURATION * SAMPLE_RATE / PLOT_STEP) {
            try {
				this->data[_channel]->remove(0);
            }
            catch (exception e) {
                LOG(WARNING) << "Exception when removing data." << std::endl;
            }
        }
        */

        // Adjust X Axis range and Y Axis range
        LOG(INFO) << "Series " << _channel << " length: " << this->data[_channel]->count() << std::endl;
        // LOG(INFO) << "here1" << std::endl;
        QList<QPointF> _tmp_data = this->data[_channel]->points();

        unsigned int _tmp_data_length = _tmp_data.length();

        // Set Y Axis
        PLC_BUFFER_TYPE _range_min = _tmp_data[0].y(), _range_max = _tmp_data[0].y();
        for (unsigned int i = 1; i < _tmp_data_length; i++) {
            PLC_BUFFER_TYPE _candidate = _tmp_data[i].y();
            if (_range_max < _candidate) {
                _range_max = _candidate;
            }
            if (_range_min > _candidate) {
                _range_min = _candidate;
            }
        }
        PLC_BUFFER_TYPE _range = _range_max - _range_min;
        this->chart[_channel]->axisY()->setMax( ceil((_range_max + 0.10f * _range) / 50) * 50 );
        this->chart[_channel]->axisY()->setMin( floor((_range_min - 0.10f * _range) / 50) * 50 );

        // LOG(INFO) << "here2" << std::endl;
        // Set X Axis
        if (_current_time > PLOT_DURATION) {
			this->chart[_channel]->axisX()->setMax(_current_time);
			this->chart[_channel]->axisX()->setMin(_current_time - PLOT_DURATION);
			// this->chart[_channel]->axisY()->setMax(4000);
        }
        else {
			this->chart[_channel]->axisX()->setMax(PLOT_DURATION);
			this->chart[_channel]->axisX()->setMin(0);
			// this->chart[_channel]->axisY()->setMax(4000);
        }

        
        // this->chart[_channel]->zoom(0.5);
        // this->chart[_channel]->zoomReset();
        // this->chart[_channel] = new QChart();
        // this->chart[_channel]->addSeries(this->data[_channel]);
        // this->chart[_channel]->createDefaultAxes();
        // this->chart[_channel]->legend()->hide();
		// this->chart[_channel]->update();
    }

    // this->ui.graphicsView->setChart(this->chart[0]);
    // this->ui.graphicsView->setRenderHint(QPainter::Antialiasing);
    // this->ui.graphicsView_2->setChart(this->chart[1]);
    // this->ui.graphicsView_2->setRenderHint(QPainter::Antialiasing);
    // this->ui.graphicsView_3->setChart(this->chart[2]);
    // this->ui.graphicsView_3->setRenderHint(QPainter::Antialiasing);
}

void  ForceMonitorQt::logData(PLC_BUFFER_TYPE** data, unsigned int* length) {
    /*
    FILE* fp = fopen("./data.csv", "a+");
	unsigned int _length = length[0];
    for (int _channel = 1; _channel < 3; _channel++) {
        if (length[_channel] < _length) {
            _length = length[_channel];
        }
    }

	for (int i = 0; i < _length; i++) {
		fprintf(fp, "%lf, %lf, %lf, \n", data[0][i], data[1][i], data[2][i]);
	}

    fclose(fp);
    */
    this->recorder->writeDataToFile(data, length);
}

void ForceMonitorQt::getDataFileName() {
    QString _filename = this->ui.lineEdit_2->text();
    if (_filename.contains(tr("[/\\~:;\",*]"))) {
        QMessageBox::critical(this, tr("Invalid Filename"), tr("Filename contains illegal characters!"), QMessageBox::Ok);
    }
    else {
        this->data_file = _filename.toStdString();
    }
}

void ForceMonitorQt::SetWorkDirectory() {
    QString work_directory = QFileDialog::getExistingDirectory();
    string _path = work_directory.toStdString();
    if (_path.length() != 0) {
		this->data_path = work_directory.toStdString();
    }
    LOG(INFO) << "Work Directory: " << this->data_path;
    // return work_directory;
}

void ForceMonitorQt::InfoWorkDirectory() {
    string _directory = this->data_path;
    string _filename = this->data_file;
    if (_filename.length() == 0) {
        _filename = "data.csv";
    }
    QMessageBox::information(this, tr("Work Directory Info"), tr(("Data saved in: " + _directory + "/" + _filename).c_str()), QMessageBox::Ok);
}

void ForceMonitorQt::AboutPage() {
    string about_info = ABOUT_PAGE_INFO;
    QMessageBox::about(this, tr("About"), tr(about_info.c_str()));
}

void ForceMonitorQt::resetCh1Offset() {
    this->data_offset_reset_flag[0] = 1;
}

void ForceMonitorQt::resetCh2Offset() {
    this->data_offset_reset_flag[1] = 1;
}

void ForceMonitorQt::resetCh3Offset() {
    this->data_offset_reset_flag[2] = 1;
}

void UpdateData(ForceMonitorQt* app) {
    app->updateData();
}

void EmitUpdateSignal(ForceMonitorQt* app) {
    // FIXME: Warning, updateData is called here
    // app->updateData();

    app->updateUi();
}

/*
void ResetCh1(ForceMonitorQt* app) {
    app->resetCh1Offset();
}

void ResetCh2(ForceMonitorQt* app) {
    app->resetCh2Offset();
}

void ResetCh3(ForceMonitorQt* app) {
    app->resetCh3Offset();
}
*/

bool checkFile(string log_file_directory, string log_file_name) {
    if (log_file_name.length() == 0) {
        log_file_name = "data.csv";
    }
	string _file = log_file_directory + "/" + log_file_name;
	FILE* _fp = NULL;
	if (_fp = fopen(_file.c_str(), "r")) {
		fclose(_fp);
		return true;
	}
	else {
		return false;
	}
}

