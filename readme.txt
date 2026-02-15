An application to manage patients, appointments, and treatments using SQLite. Supports adding, editing, deleting, and viewing records.


Features:
- Add, edit, delete patients, appointments, and treatments.
- View patients, appointments, and treatments details.
- Calendar-based appointment view for selected dates.
- Log database errors and events to clinic_debug.log.

Database:
- Uses SQLite.
- Tables: Patients, Appointments, Treatments.

DB Structure:


Patients Table
Column	Type
id	INTEGER PRIMARY KEY AUTOINCREMENT
name	TEXT
age	INTEGER
contact	TEXT
medicalHistory	TEXT



Appointments Table
Column	Type
id	INTEGER PRIMARY KEY AUTOINCREMENT
patientId	INTEGER
date	TEXT
time	TEXT
purpose	TEXT
completed	INTEGER



Treatments Table
Column	Type
id	INTEGER PRIMARY KEY AUTOINCREMENT
patientId	INTEGER
appointmentId	INTEGER
notes	TEXT
medications	TEXT