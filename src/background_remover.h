/**
 * @file background_remover.h
 * @brief Header for background removal functionality
 */

#ifndef BACKGROUND_REMOVER_H
#define BACKGROUND_REMOVER_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <Python.h>

/**
 * @class BackgroundRemover
 * @brief Provides background removal capabilities for images and videos
 * 
 * This class integrates with the rembg Python library to remove backgrounds
 * from images and videos with various configuration options.
 */
class BackgroundRemover : public QObject
{
    Q_OBJECT

public:
    explicit BackgroundRemover(QObject *parent = nullptr);
    ~BackgroundRemover();

    /**
     * @brief Initialize Python interpreter and rembg module
     * @return True if successful, false otherwise
     */
    bool initializePython();

    /**
     * @brief Remove background from an image
     * @param inputFile Path to the input image file
     * @param outputFile Path where to save the output image
     * @param alpha Whether to output with alpha channel or with a solid color background
     * @param bgColor Background color to use if alpha is false (hex format: #RRGGBB)
     * @param modelType Model type to use ('u2net', 'u2netp', 'u2net_human_seg', etc.)
     * @return True if successful, false otherwise
     */
    Q_INVOKABLE bool removeImageBackground(
        const QString& inputFile, 
        const QString& outputFile,
        bool alpha = true,
        const QString& bgColor = "#FFFFFF",
        const QString& modelType = "u2net"
    );
    
    /**
     * @brief Remove background from a video
     * @param inputFile Path to the input video file
     * @param outputFile Path where to save the output video
     * @param alpha Whether to output with alpha channel or with a solid color background
     * @param bgColor Background color to use if alpha is false (hex format: #RRGGBB)
     * @param modelType Model type to use ('u2net', 'u2netp', 'u2net_human_seg', etc.)
     * @param fps Output frames per second (0 = same as input)
     * @return True if successful, false otherwise
     */
    Q_INVOKABLE bool removeVideoBackground(
        const QString& inputFile, 
        const QString& outputFile,
        bool alpha = true,
        const QString& bgColor = "#FFFFFF",
        const QString& modelType = "u2net",
        int fps = 0
    );

    /**
     * @brief Apply green screen/chroma key effect
     * @param inputFile Path to the input video/image file
     * @param outputFile Path where to save the output file
     * @param keyColor Key color to make transparent (hex format: #RRGGBB)
     * @param similarity Similarity threshold (0.0-1.0)
     * @param smoothness Edge smoothness (0.0-1.0)
     * @param spillRemoval Spill removal amount (0.0-1.0)
     * @return True if successful, false otherwise
     */
    Q_INVOKABLE bool applyChromaKey(
        const QString& inputFile, 
        const QString& outputFile,
        const QString& keyColor = "#00FF00",
        double similarity = 0.4,
        double smoothness = 0.1,
        double spillRemoval = 0.1
    );

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
    PyObject* m_rembgModule;
    QString m_tempDir;
    QProcess m_process;

    void createTempDir();
    void cleanupTempFiles();
    bool runPythonScript(const QString& scriptContent, const QStringList& args);
    QByteArray hexToRgb(const QString& hexColor);
};

#endif // BACKGROUND_REMOVER_H
