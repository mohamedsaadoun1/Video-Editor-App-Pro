import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.3
import "ThemeManager.qml" as AppTheme

Item {
    id: aiToolsPanel
    width: parent.width
    height: parent.height

    property string activeTab: "captions"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Label {
            text: qsTr("أدوات الذكاء الاصطناعي")
            font.bold: true
            font.pixelSize: 16
            color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
        }

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            
            TabButton {
                text: qsTr("تسميات توضيحية")
                onClicked: aiToolsPanel.activeTab = "captions"
            }
            
            TabButton {
                text: qsTr("إزالة الخلفية")
                onClicked: aiToolsPanel.activeTab = "bgremoval"
            }
            
            TabButton {
                text: qsTr("إعادة تأطير ذكية")
                onClicked: aiToolsPanel.activeTab = "reframe"
            }
        }

        StackLayout {
            currentIndex: tabBar.currentIndex
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Captions Tab
            Item {
                id: captionsTab
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10
                    
                    GroupBox {
                        title: qsTr("إعدادات التسميات التوضيحية")
                        Layout.fillWidth: true
                        
                        ColumnLayout {
                            width: parent.width
                            
                            ComboBox {
                                id: languageCombo
                                Layout.fillWidth: true
                                model: ["تلقائي", "العربية", "الإنجليزية", "الفرنسية", "الألمانية", "الإسبانية"]
                                currentIndex: 0
                                
                                // Mapping to language codes
                                property var languageCodes: {
                                    "تلقائي": "auto",
                                    "العربية": "ar",
                                    "الإنجليزية": "en",
                                    "الفرنسية": "fr",
                                    "الألمانية": "de",
                                    "الإسبانية": "es"
                                }
                                
                                function getSelectedLanguageCode() {
                                    return languageCodes[model[currentIndex]];
                                }
                            }
                            
                            ComboBox {
                                id: modelSizeCombo
                                Layout.fillWidth: true
                                model: ["صغير", "متوسط", "كبير"]
                                currentIndex: 1
                                
                                // Mapping to model sizes
                                property var modelSizes: {
                                    "صغير": "small",
                                    "متوسط": "medium",
                                    "كبير": "large"
                                }
                                
                                function getSelectedModelSize() {
                                    return modelSizes[model[currentIndex]];
                                }
                            }
                            
                            CheckBox {
                                id: translateCheckBox
                                text: qsTr("ترجمة إلى العربية")
                                checked: false
                            }
                            
                            Button {
                                id: generateCaptionsButton
                                text: qsTr("إنشاء تسميات توضيحية")
                                Layout.fillWidth: true
                                
                                onClicked: {
                                    var selectedFile = "";
                                    var selectedClipId = timelineManager.selectedClipId;
                                    
                                    // Get the selected clip's file path or prompt for a file
                                    if (selectedClipId && selectedClipId !== "") {
                                        // Get file path from the selected clip
                                        selectedFile = timelineManager.getClipMediaPath(selectedClipId);
                                    } else {
                                        // No clip selected, show file dialog
                                        fileDialog.open();
                                    }
                                    
                                    if (selectedFile && selectedFile !== "") {
                                        // Call the AI service to generate captions
                                        aiServices.generateCaptions(
                                            selectedFile,
                                            languageCombo.getSelectedLanguageCode(),
                                            modelSizeCombo.getSelectedModelSize()
                                        );
                                        
                                        // Show progress dialog
                                        progressDialog.title = qsTr("إنشاء تسميات توضيحية...");
                                        progressDialog.visible = true;
                                    }
                                }
                            }
                        }
                    }
                    
                    GroupBox {
                        title: qsTr("خيارات التنسيق")
                        Layout.fillWidth: true
                        
                        GridLayout {
                            width: parent.width
                            columns: 2
                            
                            Label {
                                text: qsTr("الخط:")
                                color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                            }
                            
                            ComboBox {
                                id: fontCombo
                                Layout.fillWidth: true
                                model: Qt.fontFamilies()
                                currentIndex: model.indexOf("Arial") > -1 ? model.indexOf("Arial") : 0
                            }
                            
                            Label {
                                text: qsTr("الحجم:")
                                color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                            }
                            
                            SpinBox {
                                id: fontSizeSpinBox
                                Layout.fillWidth: true
                                from: 8
                                to: 72
                                value: 24
                            }
                            
                            Label {
                                text: qsTr("اللون:")
                                color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                            }
                            
                            RowLayout {
                                Rectangle {
                                    id: fontColorRect
                                    width: 30
                                    height: 20
                                    color: "#FFFFFF"
                                    border.color: "#000000"
                                    border.width: 1
                                    
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: colorDialog.open()
                                    }
                                }
                                
                                Button {
                                    text: qsTr("تغيير...")
                                    onClicked: colorDialog.open()
                                }
                            }
                            
                            Label {
                                text: qsTr("الموضع:")
                                color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                            }
                            
                            ComboBox {
                                id: positionCombo
                                Layout.fillWidth: true
                                model: [qsTr("أسفل"), qsTr("وسط"), qsTr("أعلى")]
                                currentIndex: 0
                            }
                        }
                    }
                }
                
                // File Dialog for selecting media file
                FileDialog {
                    id: fileDialog
                    title: qsTr("اختر ملف فيديو أو صوت")
                    nameFilters: [qsTr("ملفات الوسائط (*.mp4 *.mov *.avi *.mp3 *.wav)"), qsTr("كل الملفات (*)")]
                    
                    onAccepted: {
                        var selectedFile = fileUrl.toString();
                        selectedFile = selectedFile.replace(/^(file:\/{3})/, "");
                        
                        // Call the AI service to generate captions
                        aiServices.generateCaptions(
                            selectedFile,
                            languageCombo.getSelectedLanguageCode(),
                            modelSizeCombo.getSelectedModelSize()
                        );
                        
                        // Show progress dialog
                        progressDialog.title = qsTr("إنشاء تسميات توضيحية...");
                        progressDialog.visible = true;
                    }
                }
                
                // Color Dialog for caption text color
                ColorDialog {
                    id: colorDialog
                    title: qsTr("اختر لون النص")
                    color: fontColorRect.color
                    
                    onAccepted: {
                        fontColorRect.color = color;
                    }
                }
            }

            // Background Removal Tab
            Item {
                id: bgRemovalTab
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10
                    
                    GroupBox {
                        title: qsTr("إزالة الخلفية")
                        Layout.fillWidth: true
                        
                        ColumnLayout {
                            width: parent.width
                            
                            ComboBox {
                                id: modelTypeCombo
                                Layout.fillWidth: true
                                model: [
                                    "u2net (عام، جودة عالية)",
                                    "u2netp (سريع، جودة أقل)",
                                    "u2net_human_seg (الأشخاص فقط)"
                                ]
                                currentIndex: 0
                                
                                // Mapping to model types
                                property var modelTypes: [
                                    "u2net",
                                    "u2netp",
                                    "u2net_human_seg"
                                ]
                                
                                function getSelectedModelType() {
                                    return modelTypes[currentIndex];
                                }
                            }
                            
                            CheckBox {
                                id: alphaCheckBox
                                text: qsTr("استخدام قناة ألفا (شفافية)")
                                checked: true
                            }
                            
                            RowLayout {
                                Layout.fillWidth: true
                                enabled: !alphaCheckBox.checked
                                
                                Label {
                                    text: qsTr("لون الخلفية:")
                                    color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                                }
                                
                                Rectangle {
                                    id: bgColorRect
                                    width: 30
                                    height: 20
                                    color: "#FFFFFF"
                                    border.color: "#000000"
                                    border.width: 1
                                    
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: bgColorDialog.open()
                                    }
                                }
                                
                                Button {
                                    text: qsTr("تغيير...")
                                    onClicked: bgColorDialog.open()
                                }
                            }
                            
                            RowLayout {
                                Layout.fillWidth: true
                                
                                Button {
                                    text: qsTr("إزالة خلفية الصورة")
                                    Layout.fillWidth: true
                                    
                                    onClicked: {
                                        bgRemovalFileDialog.selectExisting = true;
                                        bgRemovalFileDialog.nameFilters = [qsTr("ملفات الصور (*.jpg *.jpeg *.png)"), qsTr("كل الملفات (*)")];
                                        bgRemovalFileDialog.title = qsTr("اختر صورة");
                                        bgRemovalFileDialog.isVideo = false;
                                        bgRemovalFileDialog.open();
                                    }
                                }
                                
                                Button {
                                    text: qsTr("إزالة خلفية الفيديو")
                                    Layout.fillWidth: true
                                    
                                    onClicked: {
                                        bgRemovalFileDialog.selectExisting = true;
                                        bgRemovalFileDialog.nameFilters = [qsTr("ملفات الفيديو (*.mp4 *.mov *.avi)"), qsTr("كل الملفات (*)")];
                                        bgRemovalFileDialog.title = qsTr("اختر فيديو");
                                        bgRemovalFileDialog.isVideo = true;
                                        bgRemovalFileDialog.open();
                                    }
                                }
                            }
                        }
                    }
                    
                    GroupBox {
                        title: qsTr("مفتاح الكروما (Chroma Key)")
                        Layout.fillWidth: true
                        
                        ColumnLayout {
                            width: parent.width
                            
                            RowLayout {
                                Layout.fillWidth: true
                                
                                Label {
                                    text: qsTr("لون الإزالة:")
                                    color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                                }
                                
                                Rectangle {
                                    id: keyColorRect
                                    width: 30
                                    height: 20
                                    color: "#00FF00" // Default green
                                    border.color: "#000000"
                                    border.width: 1
                                    
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: keyColorDialog.open()
                                    }
                                }
                                
                                Button {
                                    text: qsTr("تغيير...")
                                    onClicked: keyColorDialog.open()
                                }
                            }
                            
                            Label {
                                text: qsTr("التشابه:")
                                color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                            }
                            
                            Slider {
                                id: similaritySlider
                                Layout.fillWidth: true
                                from: 0.0
                                to: 1.0
                                value: 0.4
                                stepSize: 0.01
                                
                                Label {
                                    anchors.centerIn: parent
                                    text: similaritySlider.value.toFixed(2)
                                    color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                                }
                            }
                            
                            Label {
                                text: qsTr("النعومة:")
                                color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                            }
                            
                            Slider {
                                id: smoothnessSlider
                                Layout.fillWidth: true
                                from: 0.0
                                to: 1.0
                                value: 0.1
                                stepSize: 0.01
                                
                                Label {
                                    anchors.centerIn: parent
                                    text: smoothnessSlider.value.toFixed(2)
                                    color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                                }
                            }
                            
                            Label {
                                text: qsTr("إزالة التسرب:")
                                color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                            }
                            
                            Slider {
                                id: spillSlider
                                Layout.fillWidth: true
                                from: 0.0
                                to: 1.0
                                value: 0.1
                                stepSize: 0.01
                                
                                Label {
                                    anchors.centerIn: parent
                                    text: spillSlider.value.toFixed(2)
                                    color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                                }
                            }
                            
                            Button {
                                text: qsTr("تطبيق مفتاح الكروما")
                                Layout.fillWidth: true
                                
                                onClicked: {
                                    chromaKeyFileDialog.open();
                                }
                            }
                        }
                    }
                }
                
                // File Dialog for selecting media file for background removal
                FileDialog {
                    id: bgRemovalFileDialog
                    property bool isVideo: false
                    
                    onAccepted: {
                        var selectedFile = fileUrl.toString();
                        selectedFile = selectedFile.replace(/^(file:\/{3})/, "");
                        
                        // Get output file name
                        saveFileDialog.inputFile = selectedFile;
                        saveFileDialog.isVideo = isVideo;
                        saveFileDialog.isChromaKey = false;
                        saveFileDialog.open();
                    }
                }
                
                // File Dialog for selecting media file for chroma key
                FileDialog {
                    id: chromaKeyFileDialog
                    title: qsTr("اختر ملف فيديو أو صورة")
                    nameFilters: [qsTr("ملفات الوسائط (*.mp4 *.mov *.avi *.jpg *.jpeg *.png)"), qsTr("كل الملفات (*)")]
                    
                    onAccepted: {
                        var selectedFile = fileUrl.toString();
                        selectedFile = selectedFile.replace(/^(file:\/{3})/, "");
                        
                        // Get output file name
                        saveFileDialog.inputFile = selectedFile;
                        saveFileDialog.isVideo = selectedFile.toLowerCase().endsWith(".mp4") || 
                                               selectedFile.toLowerCase().endsWith(".mov") ||
                                               selectedFile.toLowerCase().endsWith(".avi");
                        saveFileDialog.isChromaKey = true;
                        saveFileDialog.open();
                    }
                }
                
                // File Dialog for saving output file
                FileDialog {
                    id: saveFileDialog
                    property string inputFile: ""
                    property bool isVideo: false
                    property bool isChromaKey: false
                    selectExisting: false
                    
                    onAccepted: {
                        var outputFile = fileUrl.toString();
                        outputFile = outputFile.replace(/^(file:\/{3})/, "");
                        
                        if (isChromaKey) {
                            // Apply chroma key
                            backgroundRemover.applyChromaKey(
                                inputFile,
                                outputFile,
                                keyColorRect.color,
                                similaritySlider.value,
                                smoothnessSlider.value,
                                spillSlider.value
                            );
                        } else if (isVideo) {
                            // Remove video background
                            backgroundRemover.removeVideoBackground(
                                inputFile,
                                outputFile,
                                alphaCheckBox.checked,
                                bgColorRect.color,
                                modelTypeCombo.getSelectedModelType()
                            );
                        } else {
                            // Remove image background
                            backgroundRemover.removeImageBackground(
                                inputFile,
                                outputFile,
                                alphaCheckBox.checked,
                                bgColorRect.color,
                                modelTypeCombo.getSelectedModelType()
                            );
                        }
                        
                        // Show progress dialog
                        progressDialog.title = isVideo ? 
                            qsTr("جاري إزالة خلفية الفيديو...") : 
                            qsTr("جاري إزالة خلفية الصورة...");
                        progressDialog.visible = true;
                    }
                }
                
                // Color Dialog for background color
                ColorDialog {
                    id: bgColorDialog
                    title: qsTr("اختر لون الخلفية")
                    color: bgColorRect.color
                    
                    onAccepted: {
                        bgColorRect.color = color;
                    }
                }
                
                // Color Dialog for key color
                ColorDialog {
                    id: keyColorDialog
                    title: qsTr("اختر لون المفتاح")
                    color: keyColorRect.color
                    
                    onAccepted: {
                        keyColorRect.color = color;
                    }
                }
            }

            // Smart Reframe Tab
            Item {
                id: reframeTab
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10
                    
                    GroupBox {
                        title: qsTr("إعادة تأطير ذكية")
                        Layout.fillWidth: true
                        
                        ColumnLayout {
                            width: parent.width
                            
                            Label {
                                text: qsTr("نسبة العرض المستهدفة:")
                                color: AppTheme.ThemeManager.getColor("primaryTextColor", "#FFFFFF")
                            }
                            
                            ComboBox {
                                id: aspectRatioCombo
                                Layout.fillWidth: true
                                model: [
                                    "16:9 (أفقي، عريض)",
                                    "9:16 (عمودي، للموبايل)",
                                    "1:1 (مربع)",
                                    "4:3 (تلفزيون كلاسيكي)",
                                    "21:9 (سينمائي)"
                                ]
                                currentIndex: 0
                                
                                // Mapping to aspect ratios
                                property var aspectRatios: [
                                    "16:9",
                                    "9:16",
                                    "1:1",
                                    "4:3",
                                    "21:9"
                                ]
                                
                                function getSelectedAspectRatio() {
                                    return aspectRatios[currentIndex];
                                }
                            }
                            
                            CheckBox {
                                id: followFacesCheckBox
                                text: qsTr("تتبع الوجوه في الفيديو")
                                checked: true
                            }
                            
                            Button {
                                text: qsTr("إعادة تأطير فيديو")
                                Layout.fillWidth: true
                                
                                onClicked: {
                                    reframeFileDialog.open();
                                }
                            }
                        }
                    }
                    
                    Item {
                        Layout.fillHeight: true
                    }
                }
                
                // File Dialog for selecting video for reframing
                FileDialog {
                    id: reframeFileDialog
                    title: qsTr("اختر فيديو لإعادة التأطير")
                    nameFilters: [qsTr("ملفات الفيديو (*.mp4 *.mov *.avi)"), qsTr("كل الملفات (*)")]
                    
                    onAccepted: {
                        var selectedFile = fileUrl.toString();
                        selectedFile = selectedFile.replace(/^(file:\/{3})/, "");
                        
                        // Get output file name
                        reframeSaveFileDialog.inputFile = selectedFile;
                        reframeSaveFileDialog.open();
                    }
                }
                
                // File Dialog for saving reframed video
                FileDialog {
                    id: reframeSaveFileDialog
                    property string inputFile: ""
                    title: qsTr("حفظ الفيديو المعاد تأطيره")
                    selectExisting: false
                    nameFilters: [qsTr("ملفات الفيديو (*.mp4)")]
                    
                    onAccepted: {
                        var outputFile = fileUrl.toString();
                        outputFile = outputFile.replace(/^(file:\/{3})/, "");
                        
                        // Call AI service to reframe the video
                        aiServices.smartReframe(
                            inputFile,
                            outputFile,
                            aspectRatioCombo.getSelectedAspectRatio()
                        );
                        
                        // Show progress dialog
                        progressDialog.title = qsTr("جاري إعادة تأطير الفيديو...");
                        progressDialog.visible = true;
                    }
                }
            }
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
    
    // Connect to the AI services
    Connections {
        target: aiServices
        
        function onProgressUpdate(progress, message) {
            progressBar.value = progress * 100;
            progressLabel.text = message;
        }
        
        function onProcessCompleted(success, message, outputPath) {
            progressDialog.visible = false;
            
            if (success) {
                if (outputPath && outputPath !== "") {
                    if (outputPath.toLowerCase().endsWith(".srt")) {
                        // If it's an SRT file, parse it and add captions
                        timelineManager.parseSrtAndAddCaptions(outputPath);
                    } else {
                        // For other outputs, add to project bin
                        projectManager.addMediaToProject(outputPath);
                    }
                }
                
                // Show success message
                messageDialog.title = qsTr("نجاح");
                messageDialog.text = message;
                messageDialog.standardButtons = StandardButton.Ok;
                messageDialog.icon = StandardIcon.Information;
                messageDialog.visible = true;
            } else {
                // Show error message
                messageDialog.title = qsTr("خطأ");
                messageDialog.text = message;
                messageDialog.standardButtons = StandardButton.Ok;
                messageDialog.icon = StandardIcon.Critical;
                messageDialog.visible = true;
            }
        }
    }
    
    // Connect to the background remover
    Connections {
        target: backgroundRemover
        
        function onProgressUpdate(progress, message) {
            progressBar.value = progress * 100;
            progressLabel.text = message;
        }
        
        function onProcessCompleted(success, message, outputPath) {
            progressDialog.visible = false;
            
            if (success) {
                if (outputPath && outputPath !== "") {
                    // Add to project bin
                    projectManager.addMediaToProject(outputPath);
                }
                
                // Show success message
                messageDialog.title = qsTr("نجاح");
                messageDialog.text = message;
                messageDialog.standardButtons = StandardButton.Ok;
                messageDialog.icon = StandardIcon.Information;
                messageDialog.visible = true;
            } else {
                // Show error message
                messageDialog.title = qsTr("خطأ");
                messageDialog.text = message;
                messageDialog.standardButtons = StandardButton.Ok;
                messageDialog.icon = StandardIcon.Critical;
                messageDialog.visible = true;
            }
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
