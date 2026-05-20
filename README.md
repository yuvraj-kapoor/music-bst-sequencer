# 🎵 Music BST Sequencer

A **music sequencing engine built in C using a Binary Search Tree (BST)** to store, organize, and manipulate musical notes. This project demonstrates core data structures concepts through a creative application involving music playback and transformations.

---

## 🚀 Overview

This project models musical notes as nodes in a Binary Search Tree, where each note is uniquely positioned based on its timing in a song. The BST enables efficient insertion, traversal, and modification of notes while supporting advanced operations like reversing and harmonizing a song.

---

## ✨ Features

- Insert musical notes using a unique key system
- Search and delete notes from the BST
- Perform tree traversals:
  - In-order (used for playback)
  - Pre-order
  - Post-order
- Generate a playable sequence of notes
- Reverse a song (playback in reverse order)
- Harmonize notes by adjusting pitch and timing

---

## 🧠 Key Concepts

- Binary Search Trees (BST)
- Recursion
- Tree traversal algorithms
- Dynamic memory allocation (`malloc`, `free`)
- Structs and pointers in C

---

## 🎼 Data Representation

Each note is stored as a node containing:

- `frequency` → pitch of the note  
- `bar` → position in the song  
- `index` → timing within the bar  

The BST key is calculated as:
-  key = (10 * bar) + index

This ensures that all notes are uniquely ordered within the tree.

---

## 🔄 Advanced Operations

### 🔁 Reverse Song
Reconstructs the BST so the sequence of notes plays in reverse by recalculating timing positions.

### 🎶 Harmonization
Creates additional notes by:
- Shifting pitch (frequency)
- Adjusting timing offsets

---

## ▶️ How to Run

1. Compile the program: gcc music_bst.c -o music_bst
2. Run the executable:  ./music_bst

---

## 🧪 Sample Output

In-order traversal (original):
C4 → E4 → G4

Reversed playback:
G4 → E4 → C4

---

## 📌 Notes

- This project was developed as part of a university assignment.
- Focus is on correctness, data structures, and algorithmic design rather than UI.

---

## 👨‍💻 Author

**Yuvraj Kapoor**  
Computer Science Student
