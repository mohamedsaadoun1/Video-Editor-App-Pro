/**
 * @file ai_services.cpp
 * @brief Implementation of AI services for the video editor
 */

#include "ai_services.h"
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <numpy/arrayobject.h>

AIServices::AIServices(QObject *parent) 
    : QObject(parent), m_rembgModule(nullptr), m_cvModule(nullptr)
{
    // Create temp directory for processing
    m_tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/advanced_video_editor";
    QDir dir;
    if (!dir.exists(m_tempDir)) {
        dir.mkpath(m_tempDir);
    }
    
    // Connect process signals
    connect(&m_whisperProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
            QString outputPath = handleWhisperOutput();
            emit processCompleted(true, "Caption generation completed successfully", outputPath);
        } else {
            emit processCompleted(false, "Caption generation failed with exit code: " + QString::number(exitCode));
        }
    });
    
    connect(&m_whisperProcess, &QProcess::errorOccurred, [this](QProcess::ProcessError error) {
        emit processCompleted(false, "Error in caption generation process: " + QString::number(error));
    });
    
    // Initialize Python integration
    initializePython();
}

AIServices::~AIServices()
{
    cleanupTempFiles();
    
    // Clean up Python
    if (m_rembgModule) {
        Py_DECREF(m_rembgModule);
    }
    if (m_cvModule) {
        Py_DECREF(m_cvModule);
    }
    Py_Finalize();
}

bool AIServices::initializePython()
{
    qDebug() << "Initializing Python interpreter...";
    
    // Initialize the Python interpreter
    Py_Initialize();
    
    // Import numpy array API
    import_array();
    
    // Import required modules
    try {
        // Import rembg
        PyObject* rembgName = PyUnicode_FromString("rembg");
        m_rembgModule = PyImport_Import(rembgName);
        Py_DECREF(rembgName);
        
        if (!m_rembgModule) {
            qWarning() << "Failed to import rembg module";
            PyErr_Print();
            return false;
        }
        
        // Import OpenCV
        PyObject* cvName = PyUnicode_FromString("cv2");
        m_cvModule = PyImport_Import(cvName);
        Py_DECREF(cvName);
        
        if (!m_cvModule) {
            qWarning() << "Failed to import cv2 module";
            PyErr_Print();
            return false;
        }
        
        qDebug() << "Python modules successfully imported";
        return true;
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in Python initialization:" << e.what();
        return false;
    }
}

QString AIServices::generateCaptions(const QString& inputFile, const QString& language, const QString& modelSize)
{
    qDebug() << "Generating captions for:" << inputFile;
    emit progressUpdate(0.0, "Starting caption generation...");
    
    // Validate input file
    if (!QFileInfo::exists(inputFile)) {
        emit processCompleted(false, "Input file does not exist: " + inputFile);
        return QString();
    }
    
    // Execute Whisper.cpp command
    if (!executeWhisperCommand(inputFile, language, modelSize)) {
        emit processCompleted(false, "Failed to start Whisper process");
        return QString();
    }
    
    // The result will be handled in the process finished signal
    return QString();
}

bool AIServices::executeWhisperCommand(const QString& inputFile, const QString& language, const QString& modelSize)
{
    // Prepare output path
    QFileInfo fileInfo(inputFile);
    QString outputBase = m_tempDir + "/" + fileInfo.completeBaseName();
    
    // Prepare Whisper.cpp command
    m_whisperProcess.setProgram("whisper");
    QStringList arguments;
    arguments << inputFile
              << "--output_format" << "srt"
              << "--language" << language
              << "--model" << modelSize
              << "--output_dir" << m_tempDir;
    
    m_whisperProcess.setArguments(arguments);
    
    // Start the process
    qDebug() << "Starting Whisper process with command:" << m_whisperProcess.program() << arguments.join(" ");
    m_whisperProcess.start();
    
    // Signal that process has started
    emit progressUpdate(0.1, "Processing audio...");
    
    return m_whisperProcess.waitForStarted();
}

QString AIServices::handleWhisperOutput()
{
    // Get the SRT file path
    QDir dir(m_tempDir);
    QStringList filters;
    filters << "*.srt";
    QStringList srtFiles = dir.entryList(filters, QDir::Files, QDir::Time);
    
    if (srtFiles.isEmpty()) {
        qWarning() << "No SRT files found in output directory";
        return QString();
    }
    
    // Return the most recent SRT file
    return m_tempDir + "/" + srtFiles.first();
}

bool AIServices::removeBackground(const QString& inputFile, const QString& outputFile, bool alpha)
{
    qDebug() << "Removing background from image:" << inputFile;
    emit progressUpdate(0.0, "Starting background removal...");
    
    // Validate input file
    if (!QFileInfo::exists(inputFile)) {
        emit processCompleted(false, "Input file does not exist: " + inputFile);
        return false;
    }
    
    // Check if Python modules are available
    if (!m_rembgModule) {
        emit processCompleted(false, "rembg module not initialized");
        return false;
    }
    
    try {
        // Get the remove function from rembg
        PyObject* removeFunc = PyObject_GetAttrString(m_rembgModule, "remove");
        if (!removeFunc || !PyCallable_Check(removeFunc)) {
            qWarning() << "Could not find remove function in rembg module";
            Py_XDECREF(removeFunc);
            emit processCompleted(false, "Failed to find rembg.remove function");
            return false;
        }
        
        // Load image with PIL
        PyObject* pilName = PyUnicode_FromString("PIL.Image");
        PyObject* pilModule = PyImport_Import(pilName);
        Py_DECREF(pilName);
        
        if (!pilModule) {
            qWarning() << "Failed to import PIL.Image module";
            Py_DECREF(removeFunc);
            emit processCompleted(false, "Failed to import PIL.Image module");
            return false;
        }
        
        PyObject* openFunc = PyObject_GetAttrString(pilModule, "open");
        PyObject* inputPathObj = PyUnicode_FromString(inputFile.toUtf8().constData());
        PyObject* inputImage = PyObject_CallFunctionObjArgs(openFunc, inputPathObj, NULL);
        Py_DECREF(inputPathObj);
        Py_DECREF(openFunc);
        
        if (!inputImage) {
            qWarning() << "Failed to open input image";
            Py_DECREF(removeFunc);
            Py_DECREF(pilModule);
            emit processCompleted(false, "Failed to open input image");
            return false;
        }
        
        // Call rembg.remove
        emit progressUpdate(0.3, "Processing image...");
        PyObject* outputImage = PyObject_CallFunctionObjArgs(removeFunc, inputImage, NULL);
        Py_DECREF(inputImage);
        Py_DECREF(removeFunc);
        
        if (!outputImage) {
            qWarning() << "Failed to remove background from image";
            Py_DECREF(pilModule);
            PyErr_Print();
            emit processCompleted(false, "Failed to remove background from image");
            return false;
        }
        
        // Save the output image
        emit progressUpdate(0.7, "Saving result...");
        PyObject* saveFunc = PyObject_GetAttrString(outputImage, "save");
        PyObject* outputPathObj = PyUnicode_FromString(outputFile.toUtf8().constData());
        PyObject* saveResult = PyObject_CallFunctionObjArgs(saveFunc, outputPathObj, NULL);
        Py_DECREF(outputPathObj);
        Py_DECREF(saveFunc);
        Py_DECREF(outputImage);
        
        if (!saveResult) {
            qWarning() << "Failed to save output image";
            Py_DECREF(pilModule);
            PyErr_Print();
            emit processCompleted(false, "Failed to save output image");
            return false;
        }
        
        Py_DECREF(saveResult);
        Py_DECREF(pilModule);
        
        emit progressUpdate(1.0, "Background removal completed");
        emit processCompleted(true, "Background removal completed successfully", outputFile);
        return true;
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in removeBackground:" << e.what();
        emit processCompleted(false, QString("Exception in background removal: %1").arg(e.what()));
        return false;
    }
}

bool AIServices::removeVideoBackground(const QString& inputFile, const QString& outputFile, bool alpha)
{
    qDebug() << "Removing background from video:" << inputFile;
    emit progressUpdate(0.0, "Starting video background removal...");
    
    // This is a more complex operation that requires frame-by-frame processing
    // We'll use OpenCV to read the video frames, rembg to process each frame, and then
    // OpenCV to write the output video
    
    // Validate input file
    if (!QFileInfo::exists(inputFile)) {
        emit processCompleted(false, "Input file does not exist: " + inputFile);
        return false;
    }
    
    // Check if Python modules are available
    if (!m_rembgModule || !m_cvModule) {
        emit processCompleted(false, "Required Python modules not initialized");
        return false;
    }
    
    // Create a Python script to process the video and run it
    QString scriptPath = m_tempDir + "/remove_video_bg.py";
    QFile scriptFile(scriptPath);
    if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&scriptFile);
        out << "import cv2\n";
        out << "import numpy as np\n";
        out << "from rembg import remove\n";
        out << "from PIL import Image\n";
        out << "import sys\n";
        out << "import os\n\n";
        out << "def process_video(input_file, output_file, alpha=True):\n";
        out << "    try:\n";
        out << "        # Open the input video\n";
        out << "        cap = cv2.VideoCapture(input_file)\n";
        out << "        if not cap.isOpened():\n";
        out << "            print('Error: Could not open video file')\n";
        out << "            return False\n\n";
        out << "        # Get video properties\n";
        out << "        width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))\n";
        out << "        height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))\n";
        out << "        fps = cap.get(cv2.CAP_PROP_FPS)\n";
        out << "        total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))\n\n";
        out << "        # Create output video writer\n";
        out << "        fourcc = cv2.VideoWriter_fourcc(*'mp4v')\n";
        out << "        out = cv2.VideoWriter(output_file, fourcc, fps, (width, height))\n\n";
        out << "        frame_count = 0\n";
        out << "        while cap.isOpened():\n";
        out << "            ret, frame = cap.read()\n";
        out << "            if not ret:\n";
        out << "                break\n\n";
        out << "            # Convert frame to PIL Image\n";
        out << "            pil_image = Image.fromarray(cv2.cvtColor(frame, cv2.COLOR_BGR2RGB))\n";
        out << "            \n";
        out << "            # Remove background\n";
        out << "            output_image = remove(pil_image)\n";
        out << "            \n";
        out << "            # Convert back to OpenCV format\n";
        out << "            if alpha:\n";
        out << "                # If alpha channel is desired, we need to handle it specially\n";
        out << "                result = cv2.cvtColor(np.array(output_image), cv2.COLOR_RGBA2BGRA)\n";
        out << "                # OpenCV VideoWriter doesn't support alpha, so we need to blend with white\n";
        out << "                alpha_channel = result[:, :, 3]\n";
        out << "                rgb_channels = result[:, :, :3]\n";
        out << "                white_background = np.ones_like(rgb_channels, dtype=np.uint8) * 255\n";
        out << "                alpha_factor = alpha_channel[:, :, np.newaxis].astype(np.float32) / 255.0\n";
        out << "                alpha_factor = np.concatenate((alpha_factor, alpha_factor, alpha_factor), axis=2)\n";
        out << "                result = (1 - alpha_factor) * white_background + alpha_factor * rgb_channels\n";
        out << "                result = result.astype(np.uint8)\n";
        out << "            else:\n";
        out << "                result = cv2.cvtColor(np.array(output_image), cv2.COLOR_RGBA2BGR)\n";
        out << "            \n";
        out << "            # Write the frame\n";
        out << "            out.write(result)\n";
        out << "            \n";
        out << "            # Report progress\n";
        out << "            frame_count += 1\n";
        out << "            progress = frame_count / total_frames\n";
        out << "            sys.stdout.write(f'PROGRESS:{progress:.6f}\\n')\n";
        out << "            sys.stdout.flush()\n\n";
        out << "        # Release resources\n";
        out << "        cap.release()\n";
        out << "        out.release()\n";
        out << "        return True\n";
        out << "    except Exception as e:\n";
        out << "        print(f'Error: {str(e)}')\n";
        out << "        return False\n\n";
        out << "if __name__ == '__main__':\n";
        out << "    if len(sys.argv) < 3:\n";
        out << "        print('Usage: python remove_video_bg.py input_file output_file [alpha]')\n";
        out << "        sys.exit(1)\n";
        out << "    \n";
        out << "    input_file = sys.argv[1]\n";
        out << "    output_file = sys.argv[2]\n";
        out << "    alpha = True if len(sys.argv) <= 3 or sys.argv[3].lower() == 'true' else False\n";
        out << "    \n";
        out << "    if process_video(input_file, output_file, alpha):\n";
        out << "        print('SUCCESS')\n";
        out << "        sys.exit(0)\n";
        out << "    else:\n";
        out << "        print('FAILED')\n";
        out << "        sys.exit(1)\n";
        scriptFile.close();
        
        // Execute the Python script
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);
        
        QStringList args;
        args << scriptPath << inputFile << outputFile << (alpha ? "true" : "false");
        
        process.start("python3", args);
        if (!process.waitForStarted()) {
            emit processCompleted(false, "Failed to start Python process for video background removal");
            return false;
        }
        
        // Monitor the process output for progress updates
        while (process.state() == QProcess::Running) {
            process.waitForReadyRead(100);
            QString output = process.readAllStandardOutput();
            QStringList lines = output.split("\n", Qt::SkipEmptyParts);
            
            for (const QString& line : lines) {
                if (line.startsWith("PROGRESS:")) {
                    bool ok;
                    double progress = line.mid(9).toDouble(&ok);
                    if (ok) {
                        emit progressUpdate(progress, "Processing video frames...");
                    }
                }
            }
        }
        
        process.waitForFinished();
        
        if (process.exitCode() == 0) {
            emit progressUpdate(1.0, "Video background removal completed");
            emit processCompleted(true, "Video background removal completed successfully", outputFile);
            return true;
        } else {
            QString errorOutput = process.readAllStandardOutput();
            emit processCompleted(false, "Failed to remove video background: " + errorOutput);
            return false;
        }
    } else {
        emit processCompleted(false, "Failed to create Python script for video background removal");
        return false;
    }
}

bool AIServices::smartReframe(const QString& inputFile, const QString& outputFile, const QString& targetRatio)
{
    qDebug() << "Smart reframing video:" << inputFile << "to ratio:" << targetRatio;
    emit progressUpdate(0.0, "Starting smart reframing...");
    
    // Validate input file
    if (!QFileInfo::exists(inputFile)) {
        emit processCompleted(false, "Input file does not exist: " + inputFile);
        return false;
    }
    
    // Parse target ratio
    QStringList ratioParts = targetRatio.split(":");
    if (ratioParts.size() != 2) {
        emit processCompleted(false, "Invalid aspect ratio format, should be width:height");
        return false;
    }
    
    bool okWidth, okHeight;
    int targetWidth = ratioParts[0].toInt(&okWidth);
    int targetHeight = ratioParts[1].toInt(&okHeight);
    
    if (!okWidth || !okHeight || targetWidth <= 0 || targetHeight <= 0) {
        emit processCompleted(false, "Invalid aspect ratio values");
        return false;
    }
    
    // Create a Python script to handle the smart reframing
    QString scriptPath = m_tempDir + "/smart_reframe.py";
    QFile scriptFile(scriptPath);
    if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&scriptFile);
        out << "import cv2\n";
        out << "import numpy as np\n";
        out << "import sys\n";
        out << "from moviepy.editor import VideoFileClip\n\n";
        out << "def smart_reframe(input_file, output_file, target_ratio):\n";
        out << "    try:\n";
        out << "        # Parse target ratio\n";
        out << "        target_width, target_height = map(int, target_ratio.split(':'))\n";
        out << "        target_aspect = target_width / target_height\n\n";
        out << "        # Load video\n";
        out << "        cap = cv2.VideoCapture(input_file)\n";
        out << "        if not cap.isOpened():\n";
        out << "            print('Error: Could not open video file')\n";
        out << "            return False\n\n";
        out << "        # Get original video properties\n";
        out << "        orig_width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))\n";
        out << "        orig_height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))\n";
        out << "        orig_aspect = orig_width / orig_height\n";
        out << "        fps = cap.get(cv2.CAP_PROP_FPS)\n";
        out << "        total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))\n\n";
        out << "        # Determine output dimensions (maintain original resolution)\n";
        out << "        if target_aspect > orig_aspect:\n";
        out << "            # Target is wider, crop top/bottom\n";
        out << "            out_width = orig_width\n";
        out << "            out_height = int(orig_width / target_aspect)\n";
        out << "        else:\n";
        out << "            # Target is taller, crop left/right\n";
        out << "            out_height = orig_height\n";
        out << "            out_width = int(orig_height * target_aspect)\n\n";
        out << "        # Set up face detection\n";
        out << "        face_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + 'haarcascade_frontalface_default.xml')\n\n";
        out << "        # Set up output video\n";
        out << "        fourcc = cv2.VideoWriter_fourcc(*'mp4v')\n";
        out << "        out = cv2.VideoWriter(output_file, fourcc, fps, (out_width, out_height))\n\n";
        out << "        # Process frames\n";
        out << "        frame_count = 0\n";
        out << "        while cap.isOpened():\n";
        out << "            ret, frame = cap.read()\n";
        out << "            if not ret:\n";
        out << "                break\n\n";
        out << "            # Detect faces\n";
        out << "            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)\n";
        out << "            faces = face_cascade.detectMultiScale(gray, 1.1, 4)\n\n";
        out << "            # Determine crop region\n";
        out << "            if len(faces) > 0:\n";
        out << "                # Use face locations to determine center of interest\n";
        out << "                face_centers = [(x + w//2, y + h//2) for (x, y, w, h) in faces]\n";
        out << "                avg_x = sum(x for x, y in face_centers) // len(faces)\n";
        out << "                avg_y = sum(y for x, y in face_centers) // len(faces)\n";
        out << "            else:\n";
        out << "                # No faces, use center of frame\n";
        out << "                avg_x = orig_width // 2\n";
        out << "                avg_y = orig_height // 2\n\n";
        out << "            # Calculate crop dimensions\n";
        out << "            if target_aspect > orig_aspect:\n";
        out << "                # Crop top/bottom\n";
        out << "                crop_y_start = max(0, avg_y - out_height // 2)\n";
        out << "                # Ensure we don't go beyond the frame\n";
        out << "                crop_y_start = min(crop_y_start, orig_height - out_height)\n";
        out << "                crop_y_end = crop_y_start + out_height\n";
        out << "                cropped_frame = frame[crop_y_start:crop_y_end, 0:orig_width]\n";
        out << "            else:\n";
        out << "                # Crop left/right\n";
        out << "                crop_x_start = max(0, avg_x - out_width // 2)\n";
        out << "                # Ensure we don't go beyond the frame\n";
        out << "                crop_x_start = min(crop_x_start, orig_width - out_width)\n";
        out << "                crop_x_end = crop_x_start + out_width\n";
        out << "                cropped_frame = frame[0:orig_height, crop_x_start:crop_x_end]\n\n";
        out << "            # Resize if necessary to match output dimensions\n";
        out << "            if cropped_frame.shape[1] != out_width or cropped_frame.shape[0] != out_height:\n";
        out << "                cropped_frame = cv2.resize(cropped_frame, (out_width, out_height))\n\n";
        out << "            # Write the frame\n";
        out << "            out.write(cropped_frame)\n\n";
        out << "            # Report progress\n";
        out << "            frame_count += 1\n";
        out << "            progress = frame_count / total_frames\n";
        out << "            sys.stdout.write(f'PROGRESS:{progress:.6f}\\n')\n";
        out << "            sys.stdout.flush()\n\n";
        out << "        # Release resources\n";
        out << "        cap.release()\n";
        out << "        out.release()\n\n";
        out << "        # Copy audio from original to reframed video\n";
        out << "        try:\n";
        out << "            original_clip = VideoFileClip(input_file)\n";
        out << "            reframed_clip = VideoFileClip(output_file)\n";
        out << "            reframed_clip = reframed_clip.set_audio(original_clip.audio)\n";
        out << "            temp_output = output_file + '.temp.mp4'\n";
        out << "            reframed_clip.write_videofile(temp_output, codec='libx264')\n";
        out << "            import os\n";
        out << "            os.replace(temp_output, output_file)\n";
        out << "        except Exception as e:\n";
        out << "            print(f'Warning: Could not copy audio: {str(e)}')\n\n";
        out << "        return True\n";
        out << "    except Exception as e:\n";
        out << "        print(f'Error: {str(e)}')\n";
        out << "        return False\n\n";
        out << "if __name__ == '__main__':\n";
        out << "    if len(sys.argv) < 4:\n";
        out << "        print('Usage: python smart_reframe.py input_file output_file target_ratio')\n";
        out << "        sys.exit(1)\n";
        out << "    \n";
        out << "    input_file = sys.argv[1]\n";
        out << "    output_file = sys.argv[2]\n";
        out << "    target_ratio = sys.argv[3]\n";
        out << "    \n";
        out << "    if smart_reframe(input_file, output_file, target_ratio):\n";
        out << "        print('SUCCESS')\n";
        out << "        sys.exit(0)\n";
        out << "    else:\n";
        out << "        print('FAILED')\n";
        out << "        sys.exit(1)\n";
        scriptFile.close();
        
        // Execute the Python script
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);
        
        QStringList args;
        args << scriptPath << inputFile << outputFile << targetRatio;
        
        process.start("python3", args);
        if (!process.waitForStarted()) {
            emit processCompleted(false, "Failed to start Python process for smart reframing");
            return false;
        }
        
        // Monitor the process output for progress updates
        while (process.state() == QProcess::Running) {
            process.waitForReadyRead(100);
            QString output = process.readAllStandardOutput();
            QStringList lines = output.split("\n", Qt::SkipEmptyParts);
            
            for (const QString& line : lines) {
                if (line.startsWith("PROGRESS:")) {
                    bool ok;
                    double progress = line.mid(9).toDouble(&ok);
                    if (ok) {
                        emit progressUpdate(progress, "Processing video frames...");
                    }
                }
            }
        }
        
        process.waitForFinished();
        
        if (process.exitCode() == 0) {
            emit progressUpdate(1.0, "Smart reframing completed");
            emit processCompleted(true, "Smart reframing completed successfully", outputFile);
            return true;
        } else {
            QString errorOutput = process.readAllStandardOutput();
            emit processCompleted(false, "Failed to reframe video: " + errorOutput);
            return false;
        }
    } else {
        emit processCompleted(false, "Failed to create Python script for smart reframing");
        return false;
    }
}

void AIServices::cleanupTempFiles()
{
    // Clean up temporary files if needed
    QDir tempDir(m_tempDir);
    QStringList filters;
    filters << "*.py" << "*.srt" << "*.wav" << "*.mp4" << "*.jpg" << "*.png";
    QStringList tempFiles = tempDir.entryList(filters, QDir::Files);
    
    for (const QString& file : tempFiles) {
        tempDir.remove(file);
    }
}
