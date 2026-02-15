#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QComboBox>
#include <QtUiTools/QUiLoader>
#include <QtCore/QFile>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QTimeEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QCalendarWidget>

#include <string>
#include <vector>

// --- Structs ---
struct Patient {
    int id;
    std::string name;
    int age;
    std::string contact;
    std::string medicalHistory;
};

struct Appointment {
    int id;
    int patientId;
    std::string date;
    std::string time;
    std::string purpose;
    bool completed;
};

struct Treatment {
    int id;
    int patientId;
    int appointmentId;
    std::string notes;
    std::vector<std::string> medications;
};

void writeLog(const QString &message) {
    QFile file("clinic_debug.log");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << message << "\n";
        file.close();
    }
}

void createTables(QSqlDatabase &db) {
    QSqlQuery query(db);

    if (!query.exec("CREATE TABLE IF NOT EXISTS Patients ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "name TEXT,"
                    "age INTEGER,"
                    "contact TEXT,"
                    "medicalHistory TEXT)")) {
        writeLog("Failed to create Patients table:" + query.lastError().text());
    } else writeLog("Patients table created or already exists" + query.lastError().text());

    if (!query.exec("CREATE TABLE IF NOT EXISTS Appointments ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "patientId INTEGER,"
                    "date TEXT,"
                    "time TEXT,"
                    "purpose TEXT,"
                    "completed INTEGER)")) {
        writeLog("Failed to create Appointments table:" + query.lastError().text());
    } else writeLog("Appointments table created or already exists");

    if (!query.exec("CREATE TABLE IF NOT EXISTS Treatments ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "patientId INTEGER,"
                    "appointmentId INTEGER,"
                    "notes TEXT,"
                    "medications TEXT)")) {
        writeLog("Failed to create Treatments table:" + query.lastError().text());
    } else writeLog("Treatments table created or already exists");
}

bool addPatient(QSqlDatabase &db, const Patient &p) {
    QSqlQuery query(db);

    QString name = QString::fromStdString(p.name).replace("'", "''");
    QString contact = QString::fromStdString(p.contact).replace("'", "''");
    QString medicalHistory = QString::fromStdString(p.medicalHistory).replace("'", "''");

    QString sql = QString("INSERT INTO Patients (name, age, contact, medicalHistory) "
                          "VALUES ('%1', %2, '%3', '%4')")
                          .arg(name)
                          .arg(p.age)
                          .arg(contact)
                          .arg(medicalHistory);

    if (!query.exec(sql)) {
        writeLog("Failed to add patient:" + query.lastError().text());
        return false;
    }
    return true;
}


std::vector<Patient> getAllPatients(QSqlDatabase &db) {
    std::vector<Patient> patients;
    QSqlQuery query(db);
    if (!query.exec("SELECT id, name, age, contact, medicalHistory FROM Patients")) return patients;

    while (query.next()) {
        patients.push_back(Patient{
            query.value(0).toInt(),
            query.value(1).toString().toStdString(),
            query.value(2).toInt(),
            query.value(3).toString().toStdString(),
            query.value(4).toString().toStdString()
        });
    }
    return patients;
}

bool addTreatment(QSqlDatabase &db, const Treatment &t) {
    QSqlQuery query(db);

    QString notes = QString::fromStdString(t.notes).replace("'", "''");
    QString meds;
    for (const auto &m : t.medications)
        meds += QString::fromStdString(m).replace("'", "''") + ";";

    QString sql = QString("INSERT INTO Treatments (patientId, appointmentId, notes, medications) "
                          "VALUES (%1, %2, '%3', '%4')")
                          .arg(t.patientId)
                          .arg(t.appointmentId)
                          .arg(notes)
                          .arg(meds);

    if (!query.exec(sql)) {
        writeLog("Failed to add treatment:" + query.lastError().text());
        return false;
    }
    return true;
}


std::vector<Treatment> getTreatmentsByPatient(QSqlDatabase &db, int patientId) {
    std::vector<Treatment> treatments;
    QSqlQuery query(db);
    query.prepare("SELECT id, patientId, appointmentId, notes, medications FROM Treatments WHERE patientId=?");
    query.addBindValue(patientId);
    if (!query.exec()) return treatments;

    while (query.next()) {
        std::string medsStr = query.value(4).toString().toStdString();
        std::vector<std::string> meds;
        size_t pos = 0;
        while ((pos = medsStr.find(';')) != std::string::npos) {
            meds.push_back(medsStr.substr(0, pos));
            medsStr.erase(0, pos + 1);
        }
        treatments.push_back(Treatment{
            query.value(0).toInt(),
            query.value(1).toInt(),
            query.value(2).toInt(),
            query.value(3).toString().toStdString(),
            meds
        });
    }
    return treatments;
}

bool addAppointment(QSqlDatabase &db, const Appointment &a) {
    QSqlQuery query(db);

    QString date = QString::fromStdString(a.date).replace("'", "''");
    QString time = QString::fromStdString(a.time).replace("'", "''");
    QString purpose = QString::fromStdString(a.purpose).replace("'", "''");

    QString sql = QString("INSERT INTO Appointments (patientId, date, time, purpose, completed) "
                          "VALUES (%1, '%2', '%3', '%4', %5)")
                          .arg(a.patientId)
                          .arg(date)
                          .arg(time)
                          .arg(purpose)
                          .arg(a.completed ? 1 : 0);

    if (!query.exec(sql)) {
        writeLog("Failed to add appointment:" + query.lastError().text());
        return false;
    }
    return true;
}


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("clinic.db");
    if (!db.open()) {
        return -1;
    }

    createTables(db);

    QUiLoader loader;
    QFile uiFile("leorio_clinic.ui");
    if (!uiFile.open(QFile::ReadOnly)) return -1;
    QMainWindow* window = qobject_cast<QMainWindow*>(loader.load(&uiFile));
    uiFile.close();
    if (!window) return -1;
    window->show();

    // --- Objects ---
    QListWidget *patientsList = window->findChild<QListWidget*>("patientListWidget");
    QListWidget *treatmentsList = window->findChild<QListWidget*>("treatmentListWidget");
    QPushButton *addTreatmentBtn = window->findChild<QPushButton*>("addTreatmentBtn");
    QPushButton *addAppointmentBtn = window->findChild<QPushButton*>("addAppointmentBtn");
    QPushButton *addPatientBtn = window->findChild<QPushButton*>("addPatientBtn");

    QLineEdit *patientNameLineEdit = window->findChild<QLineEdit*>("patientNameEdit");
    QSpinBox *ageSpinBox = window->findChild<QSpinBox*>("patientAgeSpinBox");
    QLineEdit *patientContactLineEdit = window->findChild<QLineEdit*>("patientContactEdit");
    QTextEdit *patientHistoryTextEdit = window->findChild<QTextEdit*>("patientMedicalHistoryEdit");

    QLineEdit *appointmentPatientId = window->findChild<QLineEdit*>("apptPatientIdEdit");
    QDateEdit *appointmentDateEdit = window->findChild<QDateEdit*>("apptDateEdit");
    QTimeEdit *appointmentTimeEdit = window->findChild<QTimeEdit*>("apptTimeEdit");
    QTextEdit *appointmentPurposeTextEdit = window->findChild<QTextEdit*>("apptPurposeEdit");
    QCheckBox *appointmentCompletedCheckBox = window->findChild<QCheckBox*>("apptCompletedCheckBox");

    QLineEdit *treatmentPatientId = window->findChild<QLineEdit*>("treatmentPatientIdEdit");
    QLineEdit *treatmentAppointmentId = window->findChild<QLineEdit*>("treatmentApptIdEdit");

    QTextEdit *treatmentNotesTextEdit = window->findChild<QTextEdit*>("treatmentNotesEdit");
    QTextEdit *treatmentMedicationsTextEdit = window->findChild<QTextEdit*>("treatmentMedicationsEdit");


    auto refreshPatients = [&]() {
        if (!patientsList || !treatmentPatientId) return;
        patientsList->clear();
        treatmentPatientId->clear();
        auto patients = getAllPatients(db);
        for (const auto &p : patients) {
            patientsList->addItem(QString::fromStdString(p.name));
        }
    };

    auto refreshAppointments = [&](const QDate &date) {
        if (!appointmentsList) return;
        appointmentsList->clear();

        QSqlQuery query(db);
        query.prepare("SELECT time, patientId, name, purpose FROM Appointments "
                    "JOIN Patients ON Appointments.patientId = Patients.id "
                    "WHERE date = ?");
        query.addBindValue(date.toString("yyyy-MM-dd"));

        if (!query.exec()) {
            writeLog("Failed to fetch appointments: " + query.lastError().text());
            return;
        }

        while (query.next()) {
            QString time = query.value(0).toString();
            int patientId = query.value(1).toInt();
            QString patientName = query.value(2).toString();
            QString purpose = query.value(3).toString();

            QString line = QString("%1 Patient (id) %2 %3 %4")
                            .arg(time)
                            .arg(patientId)
                            .arg(patientName)
                            .arg(purpose);

            appointmentsList->addItem(line);
        }
    };


    refreshPatients();
    // --- Add Patient ---

    if (addPatientBtn) {
        QObject::connect(addPatientBtn, &QPushButton::clicked, [&]() {
            QString name = patientNameLineEdit->text().trimmed();
            int age = ageSpinBox->value();
            QString contact = patientContactLineEdit->text().trimmed();
            QString medicalHistory = patientHistoryTextEdit->toPlainText().trimmed();

            if (name.isEmpty() || contact.isEmpty() || age == 0) {
                QMessageBox::warning(window, "Incomplete Data", "Please fill all required patient fields.");
                return;
            }

            Patient p;
            p.id = 0;
            p.name = name.toStdString();
            p.age = age;
            p.contact = contact.toStdString();
            p.medicalHistory = medicalHistory.toStdString();

            if (addPatient(db, p)) refreshPatients();
        });
    }

    // --- Edit Patient ---

    QPushButton *editPatientBtn = window->findChild<QPushButton*>("editPatientBtn");
    QObject::connect(editPatientBtn, &QPushButton::clicked, [&]() {
        int patientId = patientsList->currentRow() + 1;
        QString name = patientNameLineEdit->text().trimmed();
        int age = ageSpinBox->value();
        QString contact = patientContactLineEdit->text().trimmed();
        QString medicalHistory = patientHistoryTextEdit->toPlainText().trimmed();

        if (name.isEmpty() || contact.isEmpty() || age == 0) {
            QMessageBox::warning(window, "Incomplete Data", "Fill all patient fields to edit.");
            return;
        }

        QSqlQuery query(db);
        query.prepare("UPDATE Patients SET name=?, age=?, contact=?, medicalHistory=? WHERE id=?");
        query.addBindValue(name);
        query.addBindValue(age);
        query.addBindValue(contact);
        query.addBindValue(medicalHistory);
        query.addBindValue(patientId);

        if (!query.exec()) {
            writeLog("Failed to edit patient: " + query.lastError().text());
            return;
        }

        refreshPatients();
    });

    // --- Delete Patient ---
    QPushButton *deletePatientBtn = window->findChild<QPushButton*>("deletePatientBtn");
    QObject::connect(deletePatientBtn, &QPushButton::clicked, [&]() {
        int patientId = patientsList->currentRow() + 1;
        if (patientId <= 0) return;

        QSqlQuery query(db);
        query.prepare("DELETE FROM Patients WHERE id=?");
        query.addBindValue(patientId);
        if (!query.exec()) writeLog("Failed to delete patient: " + query.lastError().text());

        refreshPatients();
    });

    // --- Load data for selected patient ---

    QObject::connect(patientsList, &QListWidget::currentRowChanged, [&](int row){
        if (row < 0) return;
        auto patients = getAllPatients(db);
        if (row >= patients.size()) return;

        const Patient &p = patients[row];

        patientNameLineEdit->setText(QString::fromStdString(p.name));
        ageSpinBox->setValue(p.age);
        patientContactLineEdit->setText(QString::fromStdString(p.contact));
        patientHistoryTextEdit->setPlainText(QString::fromStdString(p.medicalHistory));
    });

    if (addAppointmentBtn) {
        QObject::connect(addAppointmentBtn, &QPushButton::clicked, [&]() {
            int patientId = appointmentPatientId->text().toInt();
            writeLog("id");
            QString date = appointmentDateEdit->date().toString("yyyy-MM-dd");
            QString time = appointmentTimeEdit->time().toString("HH:mm");
            QString purpose = appointmentPurposeTextEdit->toPlainText().trimmed();

            bool completed = false; // or read from a checkbox if you have one

            if (patientId <= 0 || date.isEmpty() || time.isEmpty() || purpose.isEmpty()) {
                QMessageBox::warning(window, "Incomplete Data", "Please fill all required appointment fields.");
                return;
            }

            Appointment a{0, patientId, date.toStdString(), time.toStdString(), purpose.toStdString(), completed};
            addAppointment(db, a);
            refreshAppointments();
        });
    }

    // --- Edit Appointment ---
    QPushButton *editAppointmentBtn = window->findChild<QPushButton*>("updateAppointmentBtn");

    QObject::connect(editAppointmentBtn, &QPushButton::clicked, [&]() {
        int patientId = appointmentPatientId->text().toInt();
        int appointmentId = appointmentPatientId->text().toInt();
        QString date = appointmentDateEdit->date().toString("yyyy-MM-dd");
        QString time = appointmentTimeEdit->time().toString("HH:mm");
        QString purpose = appointmentPurposeTextEdit->toPlainText().trimmed();
        bool completed = appointmentCompletedCheckBox->isChecked();

        QSqlQuery query(db);
        query.prepare("UPDATE Appointments SET patientId=?, date=?, time=?, purpose=?, completed=? WHERE id=?");
        query.addBindValue(patientId);
        query.addBindValue(date);
        query.addBindValue(time);
        query.addBindValue(purpose);
        query.addBindValue(completed ? 1 : 0);
        query.addBindValue(appointmentId);

        if (!query.exec()) writeLog("Failed to edit appointment: " + query.lastError().text());
        refreshAppointments();
    });

    // --- Delete Appointment ---
    QPushButton *deleteAppointmentBtn = window->findChild<QPushButton*>("deleteAppointmentBtn");
    QObject::connect(deleteAppointmentBtn, &QPushButton::clicked, [&]() {
        int appointmentId = appointmentPatientId->text().toInt();
        if (appointmentId <= 0) return;

        QSqlQuery query(db);
        query.prepare("DELETE FROM Appointments WHERE id=?");
        query.addBindValue(appointmentId);
        if (!query.exec()) writeLog("Failed to delete appointment: " + query.lastError().text());
        refreshAppointments();
    });

    QCalendarWidget *calendar = window->findChild<QCalendarWidget*>("calendarWidget");
    QListWidget *appointmentsList = window->findChild<QListWidget*>("appointmentsList");
    
    // --- Load Appointments for seleceted datee ---
    if (calendar && appointmentsList) {
        QObject::connect(calendar, &QCalendarWidget::selectionChanged, [=, &db]() {
            appointmentsList->clear();
            QDate selectedDate = calendar->selectedDate();
            QString dateStr = selectedDate.toString("yyyy-MM-dd");

            QSqlQuery query(db);
            query.prepare("SELECT a.time, a.patientId, p.name, a.purpose "
                        "FROM Appointments a "
                        "JOIN Patients p ON a.patientId = p.id "
                        "WHERE a.date = ?");
            query.addBindValue(dateStr);
            if (!query.exec()) {
                writeLog("Failed to fetch appointments: " + query.lastError().text());
                return;
            }

            while (query.next()) {
                QString time = query.value(0).toString();
                int patientId = query.value(1).toInt();
                QString patientName = query.value(2).toString();
                QString purpose = query.value(3).toString();

                QString line = QString("%1 Patient (%2) %3 %4")
                                .arg(time)
                                .arg(patientId)
                                .arg(patientName)
                                .arg(purpose);

                appointmentsList->addItem(line);
            }
            refreshAppointments();
        });
    }

    // --- Add Treatment ---

    if (addTreatmentBtn) {
        QObject::connect(addTreatmentBtn, &QPushButton::clicked, [&]() {
            int patientId = treatmentPatientId->text().toInt();
            int appointmentId = treatmentAppointmentId->text().toInt();
            QString notes = treatmentNotesTextEdit->toPlainText().trimmed();
            QString medsStr = treatmentMedicationsTextEdit->toPlainText().trimmed();

            if (patientId <= 0 || appointmentId <= 0 || notes.isEmpty() || medsStr.isEmpty()) {
                QMessageBox::warning(window, "Incomplete Data", "Please fill all required treatment fields.");
                return;
            }

            std::vector<std::string> meds;
            for (const QString &m : medsStr.split(';', Qt::SkipEmptyParts)) {
                meds.push_back(m.trimmed().toStdString());
            }

            Treatment t{0, patientId, appointmentId, notes.toStdString(), meds};
            addTreatment(db, t);

            // Refresh treatments list
            auto treatments = getTreatmentsByPatient(db, patientId);
            treatmentsList->clear();
            for (const auto &tr : treatments)
                treatmentsList->addItem(QString::fromStdString(tr.notes));
        });
    }

    // --- Edit Treatment ---
    QPushButton *editTreatmentBtn = window->findChild<QPushButton*>("updateTreatmentBtn");
    QObject::connect(editTreatmentBtn, &QPushButton::clicked, [&]() {
        int patientId = treatmentPatientId->text().toInt();
        int appointmentId = treatmentAppointmentId->text().toInt();
        QString notes = treatmentNotesTextEdit->toPlainText().trimmed();
        QString medsStr = treatmentMedicationsTextEdit->toPlainText().trimmed();

        std::vector<std::string> meds;
        for (const QString &m : medsStr.split(';', Qt::SkipEmptyParts)) {
            meds.push_back(m.trimmed().toStdString());
        }

        QSqlQuery query(db);
        query.prepare("UPDATE Treatments SET patientId=?, appointmentId=?, notes=?, medications=? WHERE patientId=? AND appointmentId=?");
        query.addBindValue(patientId);
        query.addBindValue(appointmentId);
        query.addBindValue(notes);
        query.addBindValue(medsStr);
        query.addBindValue(patientId);
        query.addBindValue(appointmentId);

        if (!query.exec()) writeLog("Failed to edit treatment: " + query.lastError().text());
    });

    // --- Delete Treatment ---
    QPushButton *deleteTreatmentBtn = window->findChild<QPushButton*>("deleteTreatmentBtn");
    QObject::connect(deleteTreatmentBtn, &QPushButton::clicked, [&]() {
        int patientId = treatmentPatientId->text().toInt();
        int appointmentId = treatmentAppointmentId->text().toInt();

        QSqlQuery query(db);
        query.prepare("DELETE FROM Treatments WHERE patientId=? AND appointmentId=?");
        query.addBindValue(patientId);
        query.addBindValue(appointmentId);

        if (!query.exec()) writeLog("Failed to delete treatment: " + query.lastError().text());
    });



    return app.exec();
}
