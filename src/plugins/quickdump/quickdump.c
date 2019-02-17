/**
 * @file
 *
 * @brief Source for quickdump plugin
 *
 * @copyright BSD License (see LICENSE.md or https://www.libelektra.org)
 *
 */

#include "quickdump.h"

#include <kdbhelper.h>

#include <kdberrors.h>
#include <stdio.h>

#include "portable/pendian.h"

static const kdb_unsigned_long_long_t MAGIC_NUMBER = 0x454b444200000001UL; // EKDB (in ASCII) + Version (1)

// keep #ifdef in sync with kdb export
#ifdef _WIN32
#define STDOUT_FILENAME ("CON")
#else
#define STDOUT_FILENAME ("/dev/stdout")
#endif

static inline bool writeUInt64 (FILE * file, kdb_unsigned_long_long_t value, Key * errorKey)
{
	kdb_unsigned_long_long_t littleEndian = htole64 (value);
	if (fwrite (&littleEndian, sizeof (kdb_unsigned_long_long_t), 1, file) < 1)
	{
		ELEKTRA_SET_ERROR (ELEKTRA_ERROR_WRITE_FAILED, errorKey, feof (file) ? "premature end of file" : "unknown error");
		return false;
	}
	return true;
}

static inline bool writeData (FILE * file, const char * data, kdb_unsigned_long_long_t size, Key * errorKey)
{
	if (!writeUInt64 (file, size, errorKey))
	{
		ELEKTRA_SET_ERROR (ELEKTRA_ERROR_WRITE_FAILED, errorKey, feof (file) ? "premature end of file" : "unknown error");
		return false;
	}

	if (size > 0)
	{
		if (fwrite (data, sizeof (char), size, file) < size)
		{
			ELEKTRA_SET_ERROR (ELEKTRA_ERROR_WRITE_FAILED, errorKey, feof (file) ? "premature end of file" : "unknown error");
			return false;
		}
	}
	return true;
}


static inline bool readUInt64 (FILE * file, kdb_unsigned_long_long_t * valuePtr, Key * errorKey)
{
	if (fread (valuePtr, sizeof (kdb_unsigned_long_long_t), 1, file) < 1)
	{
		ELEKTRA_SET_ERROR (ELEKTRA_ERROR_READ_FAILED, errorKey, "");
		return false;
	}
	*valuePtr = le64toh (*valuePtr);
	return true;
}

static inline char * readString (FILE * file, Key * errorKey)
{
	kdb_unsigned_long_long_t size;
	if (!readUInt64 (file, &size, errorKey))
	{
		ELEKTRA_SET_ERROR (ELEKTRA_ERROR_READ_FAILED, errorKey, feof (file) ? "premature end of file" : "unknown error");
		return NULL;
	}

	char * string = elektraMalloc (size + 1);
	if (fread (string, sizeof (char), size, file) < size)
	{
		ELEKTRA_SET_ERROR (ELEKTRA_ERROR_READ_FAILED, errorKey, feof (file) ? "premature end of file" : "unknown error");
		elektraFree (string);
		return NULL;
	}
	string[size] = '\0';
	return string;
}


int elektraQuickdumpGet (Plugin * handle ELEKTRA_UNUSED, KeySet * returned, Key * parentKey)
{
	if (!elektraStrCmp (keyName (parentKey), "system/elektra/modules/quickdump"))
	{
		KeySet * contract = ksNew (
			30, keyNew ("system/elektra/modules/quickdump", KEY_VALUE, "quickdump plugin waits for your orders", KEY_END),
			keyNew ("system/elektra/modules/quickdump/exports", KEY_END),
			keyNew ("system/elektra/modules/quickdump/exports/get", KEY_FUNC, elektraQuickdumpGet, KEY_END),
			keyNew ("system/elektra/modules/quickdump/exports/set", KEY_FUNC, elektraQuickdumpSet, KEY_END),
#include ELEKTRA_README (quickdump)
			keyNew ("system/elektra/modules/quickdump/infos/version", KEY_VALUE, PLUGINVERSION, KEY_END), KS_END);
		ksAppend (returned, contract);
		ksDel (contract);

		return ELEKTRA_PLUGIN_STATUS_SUCCESS;
	}
	// get all keys

	FILE * file = fopen (keyString (parentKey), "rb");

	if (file == NULL)
	{
		ELEKTRA_SET_ERROR_GET (parentKey);
		return ELEKTRA_PLUGIN_STATUS_ERROR;
	}

	kdb_unsigned_long_long_t magic;
	if (fread (&magic, sizeof (kdb_unsigned_long_long_t), 1, file) < 1)
	{
		fclose (file);
		return ELEKTRA_PLUGIN_STATUS_ERROR;
	}
	magic = be64toh (magic); // magic number is written big endian so EKDB magic string is readable

	if (magic != MAGIC_NUMBER)
	{
		fclose (file);
		return ELEKTRA_PLUGIN_STATUS_ERROR;
	}

	char c;
	while ((c = fgetc (file)) != EOF)
	{
		ungetc (c, file);

		char * name = readString (file, parentKey);
		if (name == NULL)
		{
			elektraFree (name);
			fclose (file);
			return ELEKTRA_PLUGIN_STATUS_ERROR;
		}

		char type = fgetc (file);
		if (type == EOF)
		{
			elektraFree (name);
			fclose (file);
			return ELEKTRA_PLUGIN_STATUS_ERROR;
		}

		Key * k;

		switch (type)
		{
		case 'b':
		{
			kdb_unsigned_long_long_t valueSize;
			if (!readUInt64 (file, &valueSize, parentKey))
			{
				elektraFree (name);
				fclose (file);
				return ELEKTRA_PLUGIN_STATUS_ERROR;
			}

			if (valueSize == 0)
			{
				k = keyNew (name, KEY_BINARY, KEY_SIZE, valueSize, KEY_END);
				elektraFree (name);
			}
			else
			{
				char * value = elektraMalloc (valueSize);
				if (fread (value, sizeof (char), valueSize, file) < valueSize)
				{
					elektraFree (name);
					fclose (file);
					return ELEKTRA_PLUGIN_STATUS_ERROR;
				}
				k = keyNew (name, KEY_BINARY, KEY_SIZE, valueSize, KEY_VALUE, value, KEY_END);
				elektraFree (name);
				elektraFree (value);
			}
			break;
		}
		case 's':
		{
			char * value = readString (file, parentKey);
			if (value == NULL)
			{
				elektraFree (name);
				fclose (file);
				return ELEKTRA_PLUGIN_STATUS_ERROR;
			}
			k = keyNew (name, KEY_VALUE, value, KEY_END);
			elektraFree (name);
			elektraFree (value);
			break;
		}
		default:
			elektraFree (name);
			fclose (file);
			return ELEKTRA_PLUGIN_STATUS_ERROR;
		}

		while ((c = fgetc (file)) != 0)
		{
			if (c == EOF)
			{
				fclose (file);
				return ELEKTRA_PLUGIN_STATUS_ERROR;
			}

			ungetc (c, file);

			char * metaName = readString (file, parentKey);
			if (metaName == NULL)
			{
				fclose (file);
				return ELEKTRA_PLUGIN_STATUS_ERROR;
			}

			char * metaValue = readString (file, parentKey);
			if (metaValue == NULL)
			{
				elektraFree (metaName);
				fclose (file);
				return ELEKTRA_PLUGIN_STATUS_ERROR;
			}

			keySetMeta (k, metaName, metaValue);
			elektraFree (metaName);
			elektraFree (metaValue);
		}

		ksAppendKey (returned, k);
	}

	fclose (file);

	return ELEKTRA_PLUGIN_STATUS_SUCCESS;
}

int elektraQuickdumpSet (Plugin * handle ELEKTRA_UNUSED, KeySet * returned, Key * parentKey)
{
	cursor_t cursor = ksGetCursor (returned);
	ksRewind (returned);

	FILE * file;

	// cannot open stdout for writing, because its already open
	if (elektraStrCmp (keyString (parentKey), STDOUT_FILENAME) == 0)
	{
		file = stdout;
	}
	else
	{
		file = fopen (keyString (parentKey), "wb");
	}

	if (file == NULL)
	{
		ELEKTRA_SET_ERROR_SET (parentKey);
		return ELEKTRA_PLUGIN_STATUS_ERROR;
	}

	// magic number is written big endian so EKDB magic string is readable
	kdb_unsigned_long_long_t magic = htobe64 (MAGIC_NUMBER);
	if (fwrite (&magic, sizeof (kdb_unsigned_long_long_t), 1, file) < 1)
	{
		fclose (file);
		return ELEKTRA_PLUGIN_STATUS_ERROR;
	}

	Key * cur;
	while ((cur = ksNext (returned)) != NULL)
	{
		kdb_unsigned_long_long_t nameSize = keyGetNameSize (cur) - 1;
		if (!writeData (file, keyName (cur), nameSize, parentKey))
		{
			fclose (file);
			return ELEKTRA_PLUGIN_STATUS_ERROR;
		}

		if (keyIsBinary (cur))
		{
			if (fputc ('b', file) == EOF)
			{
				fclose (file);
				return ELEKTRA_PLUGIN_STATUS_ERROR;
			}

			kdb_unsigned_long_long_t valueSize = keyGetValueSize (cur);

			char * value = NULL;
			if (valueSize > 0)
			{
				value = elektraMalloc (valueSize);
				if (keyGetBinary (cur, value, valueSize) == -1)
				{
					fclose (file);
					elektraFree (value);
					return ELEKTRA_PLUGIN_STATUS_ERROR;
				}
			}

			if (!writeData (file, value, valueSize, parentKey))
			{
				fclose (file);
				elektraFree (value);
				return ELEKTRA_PLUGIN_STATUS_ERROR;
			}
			elektraFree (value);
		}
		else
		{
			if (fputc ('s', file) == EOF)
			{
				fclose (file);
				return ELEKTRA_PLUGIN_STATUS_ERROR;
			}

			kdb_unsigned_long_long_t valueSize = keyGetValueSize (cur) - 1;
			if (!writeData (file, keyString (cur), valueSize, parentKey))
			{
				fclose (file);
				return ELEKTRA_PLUGIN_STATUS_ERROR;
			}
		}

		keyRewindMeta (cur);
		const Key * meta;
		while ((meta = keyNextMeta (cur)) != NULL)
		{
			kdb_unsigned_long_long_t metaNameSize = keyGetNameSize (meta) - 1;
			if (!writeData (file, keyName (meta), metaNameSize, parentKey))
			{
				fclose (file);
				return ELEKTRA_PLUGIN_STATUS_ERROR;
			}

			kdb_unsigned_long_long_t metaValueSize = keyGetValueSize (meta) - 1;
			if (!writeData (file, keyString (meta), metaValueSize, parentKey))
			{
				fclose (file);
				return ELEKTRA_PLUGIN_STATUS_ERROR;
			}
		}

		if (fputc (0, file) == EOF)
		{
			fclose (file);
			return ELEKTRA_PLUGIN_STATUS_ERROR;
		}
	}

	fclose (file);

	ksSetCursor (returned, cursor);

	return ELEKTRA_PLUGIN_STATUS_NO_UPDATE;
}

Plugin * ELEKTRA_PLUGIN_EXPORT (quickdump)
{
	// clang-format off
	return elektraPluginExport ("quickdump",
		ELEKTRA_PLUGIN_GET,	&elektraQuickdumpGet,
		ELEKTRA_PLUGIN_SET,	&elektraQuickdumpSet,
		ELEKTRA_PLUGIN_END);
}
