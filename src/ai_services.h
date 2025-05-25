/**
 * @file ai_services.h
 * @brief Header for AI services integration with the video editor
 */

#ifndef AI_SERVICES_H
#define AI_SERVICES_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <QVariantMap>
#include <Python.h>

/**
 * @class AIServices
 * @brief Provides AI-powered services for the video editor
 * 
 * This class integrates various AI capabilities including:
 * - Automatic captions generation using Whisper.cpp
 * - Background removal using the rembg library
 * - Smart reframing and object detection
 */
class AIServices : public QObject
{
    Q_OBJECT

public:
    explicit AIServices(QObject *parent = nullptr);
    ~AIServices();

    /**
     * @brief Initialize Python interpreter and required modules
     * @return True if successful, false otherwise
     */
    bool initializePython();

    /**
     * @brief Generate automatic captions for a given audio/video file
     * @param inputFile Path to the input audio/video file
     * @param language Language code (auto for auto-detection)
     * @param modelSize Model size to use (tiny, base, small, medium, large)
     * @return Path to the generated SRT file
     */
    Q_INVOKABLE QString generateCaptions(const QString& inputFile, const QString& language = "auto", const QString& modelSize = "medium");

    /**
     * @brief Remove background from an image
     * @param inputFile Path to the input image file
     * @param outputFile Path where to save the output image
     * @param alpha Whether to output with alpha channel or with white background
     * @return True if successful, false otherwise
     */
    Q_INVOKABLE bool removeBackground(const QString& inputFile, const QString& outputFile, bool alpha = true);
    
    /**
     * @brief Remove background from a video
     * @param inputFile Path to the input video file
     * @param outputFile Path where to save the output video
     * @param alpha Whether to output with alpha channel or with white background
     * @return True if successful, false otherwise
     */
    Q_INVOKABLE bool removeVideoBackground(const QString& inputFile, const QString& outputFile, bool alpha = true);

    /**
     * @brief Smart reframe video to a different aspect ratio
     * @param inputFile Path to the input video file
     * @param outputFile Path where to save the output video
     * @param targetRatio Target aspect ratio (e.g., "16:9", "9:16")
     * @return True if successful, false otherwise
     */
    Q_INVOKABLE bool smartReframe(const QString& inputFile, const QString& outputFile, const QString& targetRatio);

signals:
    /**
     * @brief Signal emitted during a process to indicate progress
     * @param progress Progress value from 0.0 to 1.0
     * @param message Optional status message
     */
    void progressUpdate(double progress, const QString& message = "");
    
    /**
     * @brief Signal emitted when a process completes
     * @param success Whether the process completed successfully
     * @param message Success or error message
     * @param outputPath Path to the output file if applicable
     */
    void processCompleted(bool success, const QString& message, const QString& outputPath = "");

private:
    QProcess m_whisperProcess;
    PyObject* m_rembgModule;
    PyObject* m_cvModule;
    QString m_tempDir;

    bool executeWhisperCommand(const QString& inputFile, const QString& language, const QString& modelSize);
    QString handleWhisperOutput();
    void cleanupTempFiles();
};

#endif // AI_SERVICES_H
