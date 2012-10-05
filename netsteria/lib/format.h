#ifndef FORMAT_H
#define FORMAT_H

#include <QString>


// Abstract base class for file format
// recognition, metadata extraction, and editing/conversion modules.
class FileFormat
{
public:
	enum TypeFlag {
		Unknown		= 0x0000,
		Text		= 0x0001,	// Human-readable text
		Image		= 0x0002,	// Raster image
		Drawing		= 0x0004,	// Vector drawing
		Audio		= 0x0010,	// Sampled audio clip/track
		Video		= 0x0020,	// Video/animation
		Archive		= 0x0100,	// Archive of other files
		DiskImage	= 0x0200,	// Removable media images
		Version		= 0x0400,	// Version log
	};
	Q_DECLARE_FLAGS(TypeFlags, TypeFlag)

	enum FeatureFlag {
		NoFeatures	= 0x00,
		Modify		= 0x01,	// Can modify and write back file
		Records		= 0x02,	// Format has logical record boundaries
		EmbeddedFiles	= 0x04,	// May contain embedded sub-files
		EmbeddedDirs	= 0x08,	// May contain entire directory trees
		CustomStorage	= 0x10,	// Have format-specific storage method
	};
	Q_DECLARE_FLAGS(FeatureFlags, FeatureFlag)


	// Short name of this file format
	virtual QString name() = 0;

	// Longer description of this file format
	virtual QString description() = 0;

	// List of common file extensions for this file format:
	// e.g., ".txt", ".mp3", ".wav".
	virtual QStringList extensions() = 0;

	// MIME types assigned to this file format, if any.
	virtual QStringList mimetypes() = 0;

	// Flags describing media types this format may support.
	// (Individual files might have only a subset of these media types.)
	virtual TypeFlags types() = 0;


	// Number of bytes at beginning of file needed for quick recognition,
	// -1 (default) if quick recognition not supported.
	virtual int recognitionPrefixSize();

	// Attempt to recognize this format from a given file prefix.
	// Returns 1 if file definitely appears to match this format,
	// 0 if it definitely isn't this format, -1 (default) if indefinite.
	virtual int recognize(const QByteArray &filePrefix);


	// Create a FileReader object to read or scan a particular file.
	// and insert information about it into our metadata index.
	virtual FileReader *newReader(QIODevice *infile, XXX *out);

	virtual FileWriter *newWriter(XXX *in, QIODevice *outfile);


	// Eventually, in VXA version, would like:
	// - max instruction count for quick recognition
	// - max instruction count per input byte for scanning
	// (to ensure termination, deterministic performance)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FileFormat::TypeFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(FileFormat::FeatureFlags)


class FileReader
{
	// Flags indicating what logical file components to extract in a scan
	enum Part {
		NoParts		= 0x00,
		FileData	= 0x01,	// Raw file data, suitably chunked
		MetaData	= 0x02,	// File metadata: ID3 tags, EXIF, etc.
		EmbeddedFiles	= 0x04, // Contents of embedded files
		EmbeddedDirs	= 0x04, // Embedded directory trees
		AllParts	= 0xffffffff
	};
	Q_DECLARE_FLAGS(Parts, Part)



	// Indicate which parts of the file to extract (default: AllParts).
	virtual void setParts(Parts which);

	// Set object for progress reporting
	void setProgress(...);


	bool scan();


	// Future: set a file region for incremental re-scanning?
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FileReader::Parts)


class ImageReader : public FileReader
{
	// Flags representing components of an image file to extract
	enum ImagePart {
		NoImageParts	= 0x00,
		Pixels		= 0x01,	// Decoded pixel data
		ImageData	= 0x02,	// Pure encoded image data
		Resolution	= 0x04,	// Total image size in pixels
		Thumbnail	= 0x08,	// Thumbnail image(s)
		PhotoInfo	= 0x10,	// Digital photo info (e.g., EXIF)
		AllImageParts	= 0xffffffff
	};
	Q_DECLARE_FLAGS(ImageParts, ImagePart)


	// Indicate which audio-specific components of audio file to extract
	// (default: AllImageParts).
	virtual void setImageParts(ImageParts which);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AudioFormat::AudioParts)


class AudioReader : public FileReader
{
	// Flags representing components of an audio file to extract
	enum AudioPart {
		NoAudioParts	= 0x00,
		Waveform	= 0x01,	// Decompressed audio waveform data
		AudioData	= 0x02,	// Pure encoded audio data stream
		Duration	= 0x04,	// Total play time
		Channels	= 0x08,	// Number of channels
		BitRate		= 0x10,	// Average/nominal encoding bit rate
		AllAudioParts	= 0xffffffff
	};
	Q_DECLARE_FLAGS(AudioParts, AudioPart)


	// Indicate which audio-specific components of audio file to extract
	// (default: AllAudioParts).
	virtual void setAudioParts(AudioParts which);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AudioFormat::AudioParts)


#endif	// FORMAT_H
