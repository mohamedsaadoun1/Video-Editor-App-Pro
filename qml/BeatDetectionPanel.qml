import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.3
import "ThemeManager.qml" as AppTheme

Item {
    id: beatDetectionPanel
    width: parent.width
    height: parent.height

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Label {
            text: qsTr("كشف الإيقاع والمزامنة")
            font.bold: true
            font.pixelSize: 16
            color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
        }

        GroupBox {
            title: qsTr("كشف الإيقاع")
            Layout.fillWidth: true
            
            ColumnLayout {
                width: parent.width
                
                Label {
                    text: qsTr("حد الكشف:")
                    color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                }
                
                Slider {
                    id: thresholdSlider
                    Layout.fillWidth: true
                    from: 0.0
                    to: 1.0
                    value: 0.3
                    stepSize: 0.05
                    
                    Label {
                        anchors.centerIn: parent
                        text: thresholdSlider.value.toFixed(2)
                        color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                    }
                }
                
                Button {
                    text: qsTr("كشف الإيقاع")
                    Layout.fillWidth: true
                    
                    onClicked: {
                        beatDetectionFileDialog.open();
                    }
                }
                
                Button {
                    text: qsTr("كشف السرعة (BPM)")
                    Layout.fillWidth: true
                    
                    onClicked: {
                        tempoDetectionFileDialog.open();
                    }
                }
            }
        }

        GroupBox {
            title: qsTr("المزامنة مع الإيقاع")
            Layout.fillWidth: true
            
            ColumnLayout {
                width: parent.width
                
                Label {
                    text: qsTr("الملف المصدر:")
                    color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                }
                
                RowLayout {
                    Layout.fillWidth: true
                    
                    TextField {
                        id: sourceFileTextField
                        Layout.fillWidth: true
                        readOnly: true
                        placeholderText: qsTr("اختر ملف صوت أو فيديو...")
                    }
                    
                    Button {
                        text: qsTr("تصفح...")
                        onClicked: {
                            syncSourceFileDialog.open();
                        }
                    }
                }
                
                CheckBox {
                    id: onlyStrongBeatsCheckBox
                    text: qsTr("استخدام الإيقاعات القوية فقط")
                    checked: true
                }
                
                Button {
                    text: qsTr("مزامنة المقاطع مع الإيقاع")
                    Layout.fillWidth: true
                    enabled: sourceFileTextField.text !== ""
                    
                    onClicked: {
                        // Synchronize clips with beats
                        var threshold = onlyStrongBeatsCheckBox.checked ? 0.5 : 0.3;
                        var beatTimes = beatDetector.getBeatTimes(sourceFileTextField.text, threshold);
                        
                        if (beatTimes.length > 0) {
                            timelineManager.syncClipsToBeats(beatTimes);
                            
                            // Show success message
                            messageDialog.title = qsTr("نجاح");
                            messageDialog.text = qsTr("تمت مزامنة المقاطع مع الإيقاع بنجاح");
                            messageDialog.standardButtons = StandardButton.Ok;
                            messageDialog.icon = StandardIcon.Information;
                            messageDialog.visible = true;
                        } else {
                            // Show error message
                            messageDialog.title = qsTr("خطأ");
                            messageDialog.text = qsTr("لم يتم العثور على إيقاعات في الملف المصدر");
                            messageDialog.standardButtons = StandardButton.Ok;
                            messageDialog.icon = StandardIcon.Critical;
                            messageDialog.visible = true;
                        }
                    }
                }
            }
        }

        GroupBox {
            title: qsTr("تقسيم بناءً على الإيقاع")
            Layout.fillWidth: true
            
            ColumnLayout {
                width: parent.width
                
                Label {
                    text: qsTr("ملف المصدر:")
                    color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                }
                
                RowLayout {
                    Layout.fillWidth: true
                    
                    TextField {
                        id: splitSourceFileTextField
                        Layout.fillWidth: true
                        readOnly: true
                        placeholderText: qsTr("اختر ملف صوت أو فيديو...")
                    }
                    
                    Button {
                        text: qsTr("تصفح...")
                        onClicked: {
                            splitSourceFileDialog.open();
                        }
                    }
                }
                
                Label {
                    text: qsTr("مجلد الإخراج:")
                    color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                }
                
                RowLayout {
                    Layout.fillWidth: true
                    
                    TextField {
                        id: outputDirTextField
                        Layout.fillWidth: true
                        readOnly: true
                        placeholderText: qsTr("اختر مجلد الإخراج...")
                    }
                    
                    Button {
                        text: qsTr("تصفح...")
                        onClicked: {
                            outputDirDialog.open();
                        }
                    }
                }
                
                CheckBox {
                    id: autoImportCheckBox
                    text: qsTr("استيراد المقاطع تلقائيًا للمشروع")
                    checked: true
                }
                
                Button {
                    text: qsTr("تقسيم الملف عند الإيقاعات")
                    Layout.fillWidth: true
                    enabled: splitSourceFileTextField.text !== "" && outputDirTextField.text !== ""
                    
                    onClicked: {
                        // Show progress dialog
                        progressDialog.title = qsTr("جاري تقسيم الملف...");
                        progressDialog.visible = true;
                        
                        // Split the file at beats
                        beatDetector.splitAtBeats(
                            splitSourceFileTextField.text,
                            outputDirTextField.text,
                            thresholdSlider.value
                        );
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }

    // File Dialog for beat detection
    FileDialog {
        id: beatDetectionFileDialog
        title: qsTr("اختر ملف صوت أو فيديو")
        nameFilters: [qsTr("ملفات الصوت والفيديو (*.mp3 *.wav *.mp4 *.mov)"), qsTr("كل الملفات (*)")]
        
        onAccepted: {
            var selectedFile = fileUrl.toString();
            selectedFile = selectedFile.replace(/^(file:\/{3})/, "");
            
            // Show progress dialog
            progressDialog.title = qsTr("جاري كشف الإيقاع...");
            progressDialog.visible = true;
            
            // Detect beats
            var beats = beatDetector.detectBeats(selectedFile, thresholdSlider.value);
        }
    }
    
    // File Dialog for tempo detection
    FileDialog {
        id: tempoDetectionFileDialog
        title: qsTr("اختر ملف صوت أو فيديو")
        nameFilters: [qsTr("ملفات الصوت والفيديو (*.mp3 *.wav *.mp4 *.mov)"), qsTr("كل الملفات (*)")]
        
        onAccepted: {
            var selectedFile = fileUrl.toString();
            selectedFile = selectedFile.replace(/^(file:\/{3})/, "");
            
            // Show progress dialog
            progressDialog.title = qsTr("جاري كشف السرعة...");
            progressDialog.visible = true;
            
            // Detect tempo
            var tempo = beatDetector.detectTempo(selectedFile);
        }
    }
    
    // File Dialog for sync source file
    FileDialog {
        id: syncSourceFileDialog
        title: qsTr("اختر ملف صوت أو فيديو")
        nameFilters: [qsTr("ملفات الصوت والفيديو (*.mp3 *.wav *.mp4 *.mov)"), qsTr("كل الملفات (*)")]
        
        onAccepted: {
            var selectedFile = fileUrl.toString();
            selectedFile = selectedFile.replace(/^(file:\/{3})/, "");
            sourceFileTextField.text = selectedFile;
        }
    }
    
    // File Dialog for split source file
    FileDialog {
        id: splitSourceFileDialog
        title: qsTr("اختر ملف صوت أو فيديو")
        nameFilters: [qsTr("ملفات الصوت والفيديو (*.mp3 *.wav *.mp4 *.mov)"), qsTr("كل الملفات (*)")]
        
        onAccepted: {
            var selectedFile = fileUrl.toString();
            selectedFile = selectedFile.replace(/^(file:\/{3})/, "");
            splitSourceFileTextField.text = selectedFile;
        }
    }
    
    // File Dialog for output directory
    FileDialog {
        id: outputDirDialog
        title: qsTr("اختر مجلد الإخراج")
        selectExisting: true
        selectFolder: true
        
        onAccepted: {
            var selectedDir = fileUrl.toString();
            selectedDir = selectedDir.replace(/^(file:\/{3})/, "");
            outputDirTextField.text = selectedDir;
        }
    }

    // Progress Dialog
    Dialog {
        id: progressDialog
        title: qsTr("جاري المعالجة...")
        modal: true
        visible: false
        closePolicy: Popup.NoAutoClose
        width: 400
        height: 150
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10
            
            Label {
                id: progressLabel
                text: qsTr("جاري المعالجة...")
                Layout.fillWidth: true
            }
            
            ProgressBar {
                id: progressBar
                Layout.fillWidth: true
                from: 0
                to: 100
                value: 0
            }
            
            Button {
                id: cancelButton
                text: qsTr("إلغاء")
                Layout.alignment: Qt.AlignRight
                
                onClicked: {
                    // TODO: Implement cancel functionality
                    progressDialog.visible = false;
                }
            }
        }
    }
    
    // Connect to the beat detector
    Connections {
        target: beatDetector
        
        function onProgressUpdate(progress, message) {
            progressBar.value = progress * 100;
            progressLabel.text = message;
        }
        
        function onProcessCompleted(success, message) {
            progressDialog.visible = false;
            
            // Show message dialog
            messageDialog.title = success ? qsTr("نجاح") : qsTr("خطأ");
            messageDialog.text = message;
            messageDialog.standardButtons = StandardButton.Ok;
            messageDialog.icon = success ? StandardIcon.Information : StandardIcon.Critical;
            messageDialog.visible = true;
        }
        
        function onBeatsDetected(beats) {
            // Show detected beats dialog
            if (beats.length > 0) {
                beatsDialog.beatCount = beats.length;
                beatsDialog.beatData = beats;
                beatsDialog.visible = true;
            }
        }
    }
    
    // Dialog for showing detected beats
    Dialog {
        id: beatsDialog
        title: qsTr("الإيقاعات المكتشفة")
        modal: true
        width: 500
        height: 400
        
        property int beatCount: 0
        property var beatData: []
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10
            
            Label {
                text: qsTr("تم العثور على %1 إيقاع").arg(beatsDialog.beatCount)
                font.bold: true
                Layout.fillWidth: true
            }
            
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                
                ListView {
                    model: beatsDialog.beatData
                    delegate: Rectangle {
                        width: parent.width
                        height: 30
                        color: index % 2 === 0 ? 
                            AppTheme.ThemeManager.getColor("alternateRowColor", "#2a2a2a") : 
                            AppTheme.ThemeManager.getColor("rowColor", "#333333")
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 5
                            
                            Label {
                                text: (index + 1) + "."
                                color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                                Layout.preferredWidth: 30
                            }
                            
                            Label {
                                text: modelData.toFixed(3) + " " + qsTr("ثانية")
                                color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                                Layout.fillWidth: true
                            }
                            
                            Label {
                                text: formatTime(modelData)
                                color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                                Layout.preferredWidth: 100
                            }
                        }
                    }
                }
            }
            
            Button {
                text: qsTr("إضافة علامات إيقاع للجدول الزمني")
                Layout.fillWidth: true
                
                onClicked: {
                    // Add beat markers to timeline
                    timelineManager.addBeatMarkers(beatsDialog.beatData);
                    beatsDialog.visible = false;
                    
                    // Show success message
                    messageDialog.title = qsTr("نجاح");
                    messageDialog.text = qsTr("تمت إضافة علامات الإيقاع للجدول الزمني");
                    messageDialog.standardButtons = StandardButton.Ok;
                    messageDialog.icon = StandardIcon.Information;
                    messageDialog.visible = true;
                }
            }
            
            Button {
                text: qsTr("إغلاق")
                Layout.alignment: Qt.AlignRight
                
                onClicked: {
                    beatsDialog.visible = false;
                }
            }
        }
        
        // Function to format time in MM:SS.ms format
        function formatTime(seconds) {
            var minutes = Math.floor(seconds / 60);
            var secs = Math.floor(seconds % 60);
            var ms = Math.floor((seconds % 1) * 1000);
            
            return String(minutes).padStart(2, '0') + ":" + 
                   String(secs).padStart(2, '0') + "." + 
                   String(ms).padStart(3, '0');
        }
    }
    
    // Message Dialog
    MessageDialog {
        id: messageDialog
        title: qsTr("رسالة")
        text: ""
        standardButtons: StandardButton.Ok
    }
}
