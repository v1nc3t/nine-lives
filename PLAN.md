# Nine Lives
 
Firmware for the M5Stack Cardputer (ESP32-S3), written in C++.
 
Nine apps in one pocket device: a password vault, 2FA codes, notes, voice memos, a habit tracker with checklist, a focus timer, a cat you grow, a private messenger, and a phone call pager. Most of it works offline. A small pixel cat shows what the device is doing.
 
---
 
## Hardware (M5Stack Cardputer, simple version)
 
| Part | Spec |
|---|---|
| Chip | ESP32-S3 (StampS3), 240 MHz dual-core, 8 MB flash, **no PSRAM** |
| Screen | 1.14" color LCD, 240×135 |
| Storage | microSD slot |
| Keyboard | 56 keys, QWERTY |
| Audio | Speaker and microphone |
| Wireless | Wi-Fi, Bluetooth LE |
| USB | USB-C, can act as a keyboard or a USB drive |
| Clock | BM8563 RTC |
| Battery | 120 mAh inside (+ 1400 mAh in the base on the Kit) |
 
**Limit:** no PSRAM means little RAM. Keep buffers small and read big files in pieces.
 
---
 
## How to create this plan
 
Each module is one issue, and the bullet points inside it are its tasks. Order: vault first, then tools that store things, then daily routine, then the cat game (it is rewarded by habits and the focus timer), then the messenger and the pager, which need the most outside work (network, phone link).
 
---
 
## 0. Core (shared by all modules) 
- Project setup: build system (PlatformIO), M5Cardputer library, folders, CI build
- Main menu: list of modules, keyboard navigation, status bar (time, battery, Wi-Fi, Bluetooth, USB)
- Keyboard input: key events, key repeat, shift/ctrl, text input box
- UI parts: lists, dialogs, text editor, scrolling, fonts for the small screen
- SD card layout: folders, config file, handle a missing or broken card
- Settings: brightness, screen timeout, volume, auto-lock time, Wi-Fi login, time zone, cat animation and sound on/off
- Power: sleep, wake, battery level, low battery warning
- Time: read and set the RTC, sync with NTP over Wi-Fi or from an iPhone over Bluetooth, set by hand (used by 2FA, habits, cat game and voice memos)
- Sound: play tones and chimes on the speaker (used by cat, pager, timer, messenger)
- Power saving: screen turns off when idle, lower CPU speed when idle, Wi-Fi off after sync, deep sleep when nothing is running, wake with a button
- Vault key service: one place that holds the unlocked vault key in RAM and wipes it on lock; 2FA, notes, messenger and voice memos ask it to encrypt and decrypt
- Phone link (Bluetooth): pair with an iPhone once, remember it, reconnect by itself; used by the Pager (ANCS) and for iPhone time sync (Current Time Service)

## 1. Vault (Hoard)
- Encryption: AES-256 (GCM mode, so changes to the file are detected) with mbedTLS, key made from the passphrase (PBKDF2), new random IV on every save
- File format: header, version, salt, encrypted entries, plan for future format changes
- Unlock: type the passphrase, handle wrong tries
- Entries: add, edit, delete, search, tags, notes field
- USB auto-type: the Cardputer acts as a USB keyboard and types the username, the password, or Tab; choose the keyboard layout
- Auto-lock: lock when idle, wait longer after each wrong passphrase
- Duress passphrase: a second passphrase opens a fake vault
- Password generator: choose length and character types, save into an entry
- Import and export: import from KeePass (`.kdbx`), export encrypted
- USB backup mode: show the SD card as a USB drive to copy the encrypted vault to a PC (never expose unencrypted data)
- Wi-Fi QR: show a saved Wi-Fi login as a QR code

## 2. 2FA codes (Whiskers)
- Code generator: TOTP (RFC 6238), 6 digits, 30 seconds, SHA-1 (add SHA-256/512 later if needed)
- Secret storage: encrypted with the vault key, saved on the SD card
- Code screen: list of accounts, countdown bar, new code every 30 seconds
- Accounts: add (Base32 secret), edit, delete, reorder
- Import: `otpauth://` links
- Time accuracy: use the RTC, resync with NTP, an iPhone or by hand, warn if the time looks wrong
- Optional: type the current code over USB

## 3. Notes (Scratchpad) 
- File browser: list `.txt` and `.md` files and folders on the SD card, create, rename, delete
- Editor: move the cursor, insert, delete, wrap lines on the small screen
- Auto-save: save every few seconds and on exit, safe writes so a power loss does not break the file
- Search: by file name and by text inside files
- Markdown view: show headers, lists and bold in read mode
- Encrypted notes: encrypt single notes with the vault key, shown as locked in the browser
- Quick snippets: one key types saved text (email, address, SSH key fingerprint) over USB

## 4. Voice memos (Purr) 
- Record: record from the built-in microphone to the SD card (WAV), start and stop with a key, show time and a level bar, write to the card in small pieces (no PSRAM)
- Files: name by date and time, own folder on the SD card, show free space
- Playback: play on the speaker, pause, skip forward and back, volume
- Manage: list, rename, delete
- Encrypted memos: optionally encrypt a memo with the vault key, shown as locked in the list
- Quick record: one key from the main menu starts recording
- Copy to PC: use the vault's USB backup mode to copy memos

## 5. Habits and checklist (Pawprints) 
- Habit list: add, edit and delete habits, set how often (every day, chosen weekdays, or a number of times per week)
- Today screen: the habits for today as a checklist, check one off with a key, undo
- Streaks: scheduled days done in a row for each habit, plus the best streak
- History: month view with a mark for every day a habit was done
- Checklist: one-time tasks, add, check off, delete, keep after restart
- Day change: a new day starts at midnight (setting for a later start time), based on the RTC
- Reminders: optional chime at a set time, for a habit or a task
- Storage: saved on the SD card in a small file, safe writes

## 6. Focus timer (Pounce) 
- Timer: work and break cycles, set the lengths
- Chimes: sounds for start, end and long break
- Stats: sessions done per day

## 7. Cat game (Kibble) 
- Cat in the status bar: asleep when the vault is locked, awake when it is unlocked
- Reactions: ears go up on an incoming call, small animation for success and error
- Sounds: purr on unlock, hiss on a wrong passphrase
- Setting to turn animation and sound off
- Cat stats: hunger, happiness and energy, saved on the SD card
- Care: feed, play, put to sleep
- Time passing: work out the stats from the RTC when the app opens (nothing runs while the device sleeps)
- Growing: the kitten grows into an adult cat over days of care, new picture for each stage
- Nine lives: neglect costs a life, care gives one back, the cat cannot die
- Rewards: finished focus timer sessions and checked-off habits give the cat food

## 8. Messenger (Yowl) 
Range: nearby mode (ESP-NOW) only works over a short distance, tens of metres indoors and a few hundred metres outdoors. Long distance needs the internet, so it goes through bridge mode.
 
- Contacts: add a contact with their public key, show a short fingerprint so you can check it is the right person
- Encryption: use an existing library (libsodium: X25519 and ChaCha20-Poly1305), keys kept in the vault, no home-made crypto
- Message format: one small format for every transport (sender, time, ID, text), long messages split into pieces
- Message storage: chats saved on the SD card, encrypted with the vault key, delete old messages
- Chat screen: contact list, conversation view, type with the keyboard, sound and mark for new messages
- Nearby mode: ESP-NOW between two Cardputers, no router or internet needed, with acknowledgements and retry
- Bridge mode: connect over Wi-Fi to a relay that runs on a computer (a PC, a laptop or a home server), this is the long-distance path
- Relay: a separate small program that runs on a computer and connects to the I2P network (for example an i2pd router with its SAM interface) and passes on encrypted messages, it never sees the text and keeps messages until the Cardputer connects and retries sending until the other relay answers (own repo, not part of the firmware); plan in detail later. Question: who runs the relay, each person on their own computer or one shared relay for a group of friends?
- Receiving: on/off switch like the pager (off by default), or check for messages when the app opens

## 9. Pager (Meow) 
- Pager connection: stay connected to the phone for call alerts, built on the phone link from Core
- iPhone: use ANCS (built into iOS) to get incoming calls
- Android: plan later (Android has no ANCS, so it needs another way). Question: skip Android, or test whether Gadgetbridge can talk to the Cardputer?
- Alerts: ring sound, caller name on screen, key to dismiss
- Status: Bluetooth icon in the status bar, check battery use
- Pager on/off: switch the pager on or off from the menu (off by default, so Bluetooth stays off and the device can sleep)
