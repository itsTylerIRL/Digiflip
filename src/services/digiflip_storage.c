#include "digiflip_storage.h"

#include <furi.h>
#include <storage/storage.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Every file is written to a .tmp sibling first and then renamed over the
   real one, so a power cut mid-write can't leave a half-written save. If the
   real file is ever missing or damaged, the loader falls back to the .tmp. */
#define DIGIFLIP_SAVE_PATH         APP_DATA_PATH("save.bin")
#define DIGIFLIP_SAVE_TMP          APP_DATA_PATH("save.tmp")
#define DIGIFLIP_SETTINGS_PATH     APP_DATA_PATH("settings.bin")
#define DIGIFLIP_SETTINGS_TMP      APP_DATA_PATH("settings.tmp")
#define DIGIFLIP_ALBUM_PATH        APP_DATA_PATH("album.bin")
#define DIGIFLIP_ALBUM_TMP         APP_DATA_PATH("album.tmp")
#define DIGIFLIP_SETTINGS_MAGIC    0x54534644UL
/* v2 added the cheat toggles, v3 the all-eggs cheat; older files load with
   the newer settings off. */
#define DIGIFLIP_SETTINGS_VERSION  3U
#define DIGIFLIP_SETTINGS_V1_SIZE  2U
#define DIGIFLIP_SETTINGS_V2_SIZE  4U
#define DIGIFLIP_RECORDS_PATH      APP_DATA_PATH("records.bin")
#define DIGIFLIP_RECORDS_TMP       APP_DATA_PATH("records.tmp")
#define DIGIFLIP_RECORDS_MAGIC     0x44434552UL
#define DIGIFLIP_RECORDS_VERSION   1U
#define DIGIFLIP_LOG_PATH          APP_DATA_PATH("log.bin")
#define DIGIFLIP_LOG_TMP           APP_DATA_PATH("log.tmp")
#define DIGIFLIP_LOG_TEXT_PATH     APP_DATA_PATH("log.txt")
#define DIGIFLIP_LOG_TEXT_TMP      APP_DATA_PATH("log.txt.tmp")
#define DIGIFLIP_LOG_MAGIC         0x474F4C44UL
#define DIGIFLIP_LOG_VERSION       1U
#define DIGIFLIP_ALBUM_MAGIC       0x4C414644UL
#define DIGIFLIP_ALBUM_VERSION     1U
#define DIGIFLIP_SAVE_MAGIC        0x50494C46UL
#define DIGIFLIP_SAVE_VERSION      7U
/* v4 is v5 minus the fields from DIGIFLIP_GAME_V4_TAIL onward. */
#define DIGIFLIP_SAVE_V4_GAME_SIZE offsetof(DigiflipGame, DIGIFLIP_GAME_V4_TAIL)
#define DIGIFLIP_SAVE_V5_GAME_SIZE offsetof(DigiflipGame, DIGIFLIP_GAME_V5_TAIL)
#define DIGIFLIP_SAVE_V6_GAME_SIZE offsetof(DigiflipGame, DIGIFLIP_GAME_V6_TAIL)
#define DIGIFLIP_SAVE2_PATH        APP_DATA_PATH("save2.bin")
#define DIGIFLIP_SAVE2_TMP         APP_DATA_PATH("save2.tmp")
#define DIGIFLIP_SLOT2_PATH        APP_DATA_PATH("slot2.bin")
#define DIGIFLIP_SLOT2_TMP         APP_DATA_PATH("slot2.tmp")
#define DIGIFLIP_SLOT2_MAGIC       0x32544C53UL

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    DigiflipGame game;
    uint32_t checksum;
} DigiflipSave;

static uint32_t digiflip_checksum(const void* data, size_t size) {
    const uint8_t* bytes = data;
    uint32_t hash = 2166136261UL;
    for(size_t i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= 16777619UL;
    }
    return hash;
}

/* Write `size` bytes to `tmp_path`, flush, then rename over `path`. */
static bool
    digiflip_write_atomic(const char* path, const char* tmp_path, const void* data, size_t size) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    storage_common_mkdir(storage, APP_DATA_PATH(""));
    bool written = false;
    if(storage_file_open(file, tmp_path, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        written = storage_file_write(file, data, size) == size && storage_file_sync(file);
    }
    storage_file_close(file);
    storage_file_free(file);
    const bool saved = written && storage_common_rename(storage, tmp_path, path) == FSE_OK;
    furi_record_close(RECORD_STORAGE);
    return saved;
}

/* Reads the header, then `size` bytes of game, then the checksum. Older
   versions whose game struct is a prefix of the current one are migrated by
   zeroing the new tail fields. */
static bool digiflip_read_save(Storage* storage, const char* path, DigiflipGame* game) {
    File* file = storage_file_alloc(storage);
    DigiflipSave save;
    bool loaded = false;

    if(storage_file_open(file, path, FSAM_READ, FSOM_OPEN_EXISTING)) {
        const size_t header = offsetof(DigiflipSave, game);
        memset(&save, 0, sizeof(save));
        if(storage_file_read(file, &save, header) == header && save.magic == DIGIFLIP_SAVE_MAGIC) {
            /* Older saves are a prefix of today's struct: keep the fields they
               had and zero the ones added since (their tail may be padding). */
            size_t keep = 0;
            if(save.version == DIGIFLIP_SAVE_VERSION && save.size == sizeof(save.game)) {
                keep = sizeof(save.game);
            } else if(save.version == 6U) {
                keep = DIGIFLIP_SAVE_V6_GAME_SIZE;
            } else if(save.version == 5U) {
                keep = DIGIFLIP_SAVE_V5_GAME_SIZE;
            } else if(save.version == 4U) {
                keep = DIGIFLIP_SAVE_V4_GAME_SIZE;
            }
            const bool readable = keep && save.size >= keep && save.size <= sizeof(save.game);
            uint32_t checksum = 0;
            if(readable && storage_file_read(file, &save.game, save.size) == save.size &&
               storage_file_read(file, &checksum, sizeof(checksum)) == sizeof(checksum) &&
               checksum == digiflip_checksum(&save.game, save.size)) {
                memset((uint8_t*)&save.game + keep, 0, sizeof(save.game) - keep);
                *game = save.game;
                loaded = true;
            }
        }
    }

    storage_file_close(file);
    storage_file_free(file);
    return loaded;
}

bool digiflip_storage_load_slot(DigiflipGame* game, uint8_t slot) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    const char* path = slot ? DIGIFLIP_SAVE2_PATH : DIGIFLIP_SAVE_PATH;
    const char* tmp = slot ? DIGIFLIP_SAVE2_TMP : DIGIFLIP_SAVE_TMP;
    const bool loaded = digiflip_read_save(storage, path, game) ||
                        digiflip_read_save(storage, tmp, game);
    furi_record_close(RECORD_STORAGE);
    return loaded;
}

bool digiflip_storage_save_slot(const DigiflipGame* game, uint8_t slot) {
    const DigiflipSave save = {
        .magic = DIGIFLIP_SAVE_MAGIC,
        .version = DIGIFLIP_SAVE_VERSION,
        .size = sizeof(*game),
        .game = *game,
        .checksum = digiflip_checksum(game, sizeof(*game)),
    };
    return digiflip_write_atomic(
        slot ? DIGIFLIP_SAVE2_PATH : DIGIFLIP_SAVE_PATH,
        slot ? DIGIFLIP_SAVE2_TMP : DIGIFLIP_SAVE_TMP,
        &save,
        sizeof(save));
}

/* Read up to `size` bytes (the rest zeroed) from the first of `path` /
   `tmp_path` that the caller's `valid` check accepts. Older, shorter files
   are told apart by their header. */
static bool digiflip_read_small(
    const char* path,
    const char* tmp_path,
    void* data,
    size_t size,
    bool (*valid)(const void* data)) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    bool loaded = false;
    const char* paths[] = {path, tmp_path};
    for(size_t i = 0; i < 2 && !loaded; i++) {
        if(storage_file_open(file, paths[i], FSAM_READ, FSOM_OPEN_EXISTING)) {
            memset(data, 0, size);
            loaded = storage_file_read(file, data, size) >= 8U && valid(data);
        }
        storage_file_close(file);
    }
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    return loaded;
}

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    DigiflipSettings settings;
} DigiflipSettingsFile;

void digiflip_settings_defaults(DigiflipSettings* settings) {
    memset(settings, 0, sizeof(*settings));
    settings->sound = true;
    settings->call_light = true;
}

static bool digiflip_settings_valid(const void* data) {
    const DigiflipSettingsFile* file = data;
    if(file->magic != DIGIFLIP_SETTINGS_MAGIC) return false;
    return (file->version == DIGIFLIP_SETTINGS_VERSION && file->size == sizeof(file->settings)) ||
           (file->version == 2U && file->size == DIGIFLIP_SETTINGS_V2_SIZE) ||
           (file->version == 1U && file->size == DIGIFLIP_SETTINGS_V1_SIZE);
}

bool digiflip_settings_load(DigiflipSettings* settings) {
    DigiflipSettingsFile data;
    digiflip_settings_defaults(settings);
    const bool loaded = digiflip_read_small(
        DIGIFLIP_SETTINGS_PATH,
        DIGIFLIP_SETTINGS_TMP,
        &data,
        sizeof(data),
        digiflip_settings_valid);
    if(loaded) {
        if(data.version == 1U) {
            settings->sound = data.settings.sound;
            settings->call_light = data.settings.call_light;
        } else {
            *settings = data.settings;
            if(data.version == 2U) settings->cheat_all_eggs = false;
        }
    }
    return loaded;
}

bool digiflip_settings_save(const DigiflipSettings* settings) {
    const DigiflipSettingsFile data = {
        .magic = DIGIFLIP_SETTINGS_MAGIC,
        .version = DIGIFLIP_SETTINGS_VERSION,
        .size = sizeof(*settings),
        .settings = *settings,
    };
    return digiflip_write_atomic(
        DIGIFLIP_SETTINGS_PATH, DIGIFLIP_SETTINGS_TMP, &data, sizeof(data));
}

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    DigiflipAlbum album;
    uint32_t checksum;
} DigiflipAlbumFile;

static bool digiflip_album_valid(const void* data) {
    const DigiflipAlbumFile* file = data;
    return file->magic == DIGIFLIP_ALBUM_MAGIC && file->version == DIGIFLIP_ALBUM_VERSION &&
           file->size == sizeof(file->album) &&
           file->checksum == digiflip_checksum(&file->album, sizeof(file->album));
}

bool digiflip_album_load(DigiflipAlbum* album) {
    DigiflipAlbumFile data;
    digiflip_album_clear(album);
    const bool loaded = digiflip_read_small(
        DIGIFLIP_ALBUM_PATH, DIGIFLIP_ALBUM_TMP, &data, sizeof(data), digiflip_album_valid);
    if(loaded) *album = data.album;
    return loaded;
}

bool digiflip_album_save(const DigiflipAlbum* album) {
    const DigiflipAlbumFile data = {
        .magic = DIGIFLIP_ALBUM_MAGIC,
        .version = DIGIFLIP_ALBUM_VERSION,
        .size = sizeof(*album),
        .album = *album,
        .checksum = digiflip_checksum(album, sizeof(*album)),
    };
    return digiflip_write_atomic(DIGIFLIP_ALBUM_PATH, DIGIFLIP_ALBUM_TMP, &data, sizeof(data));
}

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    DigiflipLog log;
    uint32_t checksum;
} DigiflipLogFile;

static bool digiflip_log_valid(const void* data) {
    const DigiflipLogFile* file = data;
    return file->magic == DIGIFLIP_LOG_MAGIC && file->version == DIGIFLIP_LOG_VERSION &&
           file->size == sizeof(file->log) && file->log.count <= DIGIFLIP_LOG_CAPACITY &&
           file->log.head < DIGIFLIP_LOG_CAPACITY &&
           file->checksum == digiflip_checksum(&file->log, sizeof(file->log));
}

/* The log file is ~2 KB, so it goes through a heap buffer, not the stack. */
bool digiflip_log_load(DigiflipLog* log) {
    DigiflipLogFile* data = malloc(sizeof(DigiflipLogFile));
    digiflip_log_clear(log);
    if(!data) return false;
    const bool loaded = digiflip_read_small(
        DIGIFLIP_LOG_PATH, DIGIFLIP_LOG_TMP, data, sizeof(*data), digiflip_log_valid);
    if(loaded) *log = data->log;
    free(data);
    return loaded;
}

bool digiflip_log_save(const DigiflipLog* log) {
    DigiflipLogFile* data = malloc(sizeof(DigiflipLogFile));
    if(!data) return false;
    data->magic = DIGIFLIP_LOG_MAGIC;
    data->version = DIGIFLIP_LOG_VERSION;
    data->size = sizeof(*log);
    data->log = *log;
    data->checksum = digiflip_checksum(log, sizeof(*log));
    const bool saved =
        digiflip_write_atomic(DIGIFLIP_LOG_PATH, DIGIFLIP_LOG_TMP, data, sizeof(*data));
    free(data);
    return saved;
}

static bool digiflip_write_line(File* file, const FuriString* line) {
    return storage_file_write(file, furi_string_get_cstr(line), furi_string_size(line)) ==
           furi_string_size(line);
}

bool digiflip_log_export(const DigiflipLog* log) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    FuriString* line = furi_string_alloc();
    char when[24];
    char what[48];
    bool written = false;
    storage_common_mkdir(storage, APP_DATA_PATH(""));
    if(storage_file_open(file, DIGIFLIP_LOG_TEXT_TMP, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        furi_string_printf(line, "DigiFlip log, %u events, oldest first\n", log->count);
        written = digiflip_write_line(file, line);
        for(uint16_t i = log->count; written && i > 0; i--) {
            const DigiflipEvent* event = digiflip_log_get(log, (uint16_t)(i - 1U));
            digiflip_time_long(event->time, when, sizeof(when));
            digiflip_event_describe(event, what, sizeof(what));
            furi_string_printf(line, "%s  %s%s", when, event->slot == 1 ? "[pet 2] " : "", what);
            if(event->type == DigiflipEventEvolved) {
                const DigiflipSpecies* from = digiflip_species_get(event->b);
                furi_string_cat_printf(line, " (from %s)", from ? from->name : "?");
            }
            /* Raw fields too, for debugging the rules. */
            furi_string_cat_printf(
                line, "  [type %u a=%u b=%u c=%u]\n", event->type, event->a, event->b, event->c);
            written = digiflip_write_line(file, line);
        }
        written = written && storage_file_sync(file);
    }
    storage_file_close(file);
    storage_file_free(file);
    furi_string_free(line);
    const bool saved =
        written &&
        storage_common_rename(storage, DIGIFLIP_LOG_TEXT_TMP, DIGIFLIP_LOG_TEXT_PATH) == FSE_OK;
    furi_record_close(RECORD_STORAGE);
    return saved;
}

typedef struct {
    uint32_t magic;
    uint8_t version;
    uint8_t kind; /* DigiflipSlotKind */
    uint16_t copymon;
    uint32_t checksum;
} DigiflipSlot2File;

static uint32_t digiflip_slot2_sum(const DigiflipSlot2File* file) {
    const uint32_t fields = (uint32_t)file->kind | ((uint32_t)file->copymon << 8);
    return digiflip_checksum(&fields, sizeof(fields));
}

static bool digiflip_slot2_valid(const void* data) {
    const DigiflipSlot2File* file = data;
    return file->magic == DIGIFLIP_SLOT2_MAGIC && file->version == 1U &&
           file->kind <= DigiflipSlotCopymon && file->checksum == digiflip_slot2_sum(file);
}

bool digiflip_slot2_load(DigiflipSlotKind* kind, DigiflipSpeciesId* copymon) {
    DigiflipSlot2File data;
    *kind = DigiflipSlotEmpty;
    *copymon = DIGIFLIP_SPECIES_NONE;
    if(!digiflip_read_small(
           DIGIFLIP_SLOT2_PATH, DIGIFLIP_SLOT2_TMP, &data, sizeof(data), digiflip_slot2_valid))
        return false;
    *kind = (DigiflipSlotKind)data.kind;
    if(*kind == DigiflipSlotCopymon) {
        if(digiflip_species_get(data.copymon)) {
            *copymon = data.copymon;
        } else {
            *kind = DigiflipSlotEmpty;
        }
    }
    return true;
}

bool digiflip_slot2_save(DigiflipSlotKind kind, DigiflipSpeciesId copymon) {
    DigiflipSlot2File data = {
        .magic = DIGIFLIP_SLOT2_MAGIC,
        .version = 1U,
        .kind = (uint8_t)kind,
        .copymon = copymon,
    };
    data.checksum = digiflip_slot2_sum(&data);
    return digiflip_write_atomic(DIGIFLIP_SLOT2_PATH, DIGIFLIP_SLOT2_TMP, &data, sizeof(data));
}

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    DigiflipRecords records;
    uint32_t checksum;
} DigiflipRecordsFile;

static bool digiflip_records_valid(const void* data) {
    const DigiflipRecordsFile* file = data;
    return file->magic == DIGIFLIP_RECORDS_MAGIC && file->version == DIGIFLIP_RECORDS_VERSION &&
           file->size == sizeof(file->records) &&
           file->checksum == digiflip_checksum(&file->records, sizeof(file->records));
}

bool digiflip_records_load(DigiflipRecords* records) {
    DigiflipRecordsFile data;
    memset(records, 0, sizeof(*records));
    const bool loaded = digiflip_read_small(
        DIGIFLIP_RECORDS_PATH, DIGIFLIP_RECORDS_TMP, &data, sizeof(data), digiflip_records_valid);
    if(loaded) *records = data.records;
    return loaded;
}

bool digiflip_records_save(const DigiflipRecords* records) {
    const DigiflipRecordsFile data = {
        .magic = DIGIFLIP_RECORDS_MAGIC,
        .version = DIGIFLIP_RECORDS_VERSION,
        .size = sizeof(*records),
        .records = *records,
        .checksum = digiflip_checksum(records, sizeof(*records)),
    };
    return digiflip_write_atomic(DIGIFLIP_RECORDS_PATH, DIGIFLIP_RECORDS_TMP, &data, sizeof(data));
}
