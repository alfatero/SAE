from PyQt5.QtWidgets import QApplication, QWidget, QFileDialog, QLabel, QVBoxLayout, QPushButton

def open_directory_dialog():
    app = QApplication([])

    directory = QFileDialog.getExistingDirectory(None, 'Choisir un répertoire')

    if directory:
        return directory
    else:
        return None

