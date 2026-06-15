# mypstree — Réimplémentation de la commande pstree sous Linux

`mypstree` est un utilitaire système développé en C sous Linux permettant d'afficher l'arbre des processus en cours d'exécution. En interrogeant directement le système de fichiers virtuel `/proc` du noyau, le programme extrait les métadonnées des processus vivants, reconstruit dynamiquement leur arbre généalogique (relations parent-enfant) et génère un rendu visuel hiérarchique structuré.

Ce projet a été conçu selon une approche modulaire stricte, respectant une séparation nette entre l'acquisition des données, la structuration algorithmique et la couche d'affichage.

---

## 🚀 Fonctionnalités et Options CLI

L'application prend en charge la syntaxe standard `mypstree [options] [PID]` et intègre 11 options en ligne de commande :

### Options de Base
* `-p` : **Affichage des PIDs** — Ajoute l'identifiant numérique du processus à côté de son nom.
* `-a` : **Arguments de ligne de commande** — Extrait et affiche la ligne de commande complète depuis `/proc/[pid]/cmdline`, en gérant les séparateurs d'octets nuls.
* `-n` : **Tri Alphabétique** — Trie les processus enfants par nom au lieu de l'ordre d'apparition dans `/proc`.
* `-h` : **Mise en évidence (Highlight)** — Met en valeur graphiquement (via codes ANSI gras) le processus appelant actuel et sa lignée d'ancêtres.
* `-c` : **Désactivation du compactage** — Conserve l'arborescence complète (option structurée pour l'affichage de sous-arbres).
* `-H` : **Aide** — Affiche le manuel d'utilisation des options.

### Options Avancées (Ajouts Spécifiques)
* `-A` : **Tracé ASCII** — Substitue les caractères graphiques UTF-8 par des symboles purement ASCII (`|-`, `` `- ``, `| `) pour assurer une compatibilité parfaite avec les terminaux anciens ou minimalistes.
* `-g` : **Affichage du PGID** — Extrait et affiche le *Process Group ID* (identifiant de groupe de processus) depuis `/proc/[pid]/stat`.
* `-s` : **Lignée des parents** — Isole et affiche uniquement la lignée ascendante (les parents directs) d'un PID cible jusqu'à la racine (PID 1).
* `-T` : **Masquage des threads** — Filtre les flux légers d'exécution (threads) pour ne conserver que les processus lourds distincts en comparant `Pid` et `Tgid`.
* `-V` : **Version** — Affiche la version courante de l'utilitaire système.

---

## 📂 Architecture Modulaire du Code

Le projet est découpé de manière chirurgicale en modules isolés possédant chacun une responsabilité unique :


```

mypstree/
├── main.c           # Orchestrateur central de l'application
├── options.h/.c     # Analyseur d'arguments CLI via la fonction getopt()
├── proc_reader.h/.c # Couche de données (Parsing de /proc/[pid]/status, stat, cmdline)
├── tree_builder.h/.c# Gestion algorithmique de l'arbre (Tableaux dynamiques, qsort, élagage)
├── tree_printer.h/.c# Couche de rendu visuel (Parcours DFS, gestion ANSI et UTF-8/ASCII)
└── Makefile         # Script d'automatisation de la compilation

```

### Détails des Composants
1.  **`options`** : Configure la structure de configuration globale en fonction des drapeaux activés ou des arguments positionnels (PID cible).
2.  **`proc_reader`** : Parcourt le dossier `/proc`, valide les dossiers numériques, extrait de manière sécurisée les tokens requis et prévient les *conditions de course* (si un processus s'éteint pendant sa lecture, le pointeur de fichier `NULL` est intercepté sans plantage).
3.  **`tree_builder`** : Alloue la mémoire dynamiquement pour les nœuds. Utilise un redimensionnement géométrique (`realloc` doublant la capacité) pour stocker les listes d'enfants. Gère l'élagage récursif pour l'option de filiation `-s`.
4.  **`tree_printer`** : Utilise un algorithme de parcours en profondeur d'abord (DFS) pour imprimer le graphique textuel. Il maintient un tableau de préfixes pour dessiner précisément les branches descendantes sans orphelin visuel.

---

## 🛠️ Compilation et Installation

Un `Makefile` robuste est fourni à la racine. Il applique des règles strictes de compilation (`-Wall -Wextra -O2`) afin de maximiser la sécurité et la performance du binaire.

Pour compiler le projet, exécutez simplement :
```bash
make

```

Cela générera un exécutable nommé `mypstree`.

Pour nettoyer les fichiers objets (`.o`) et repartir sur une configuration propre :

```bash
make clean

```

---

## 💡 Exemples d'Utilisation

**1. Affichage standard avec PIDs :**

```bash
./mypstree -p

```

**2. Tri par nom avec arguments complets du terminal en mode ASCII :**

```bash
./mypstree -anA

```

**3. Isoler uniquement la lignée parente du processus numéro 4242 :**

```bash
./mypstree -s 4242

```

**4. Afficher l'arbre global sans les threads en incluant le PGID :**

```bash
./mypstree -Tg

```

---

## 🛡️ Gestion de la Mémoire et Qualité

L'application a été auditée et nettoyée pour éviter toute fuite ou corruption mémoire. La désallocation complète de l'arbre est assurée par une fonction de libération récursive (`free_tree()`) qui nettoie chaque nœud enfant et ses tableaux dynamiques de pointeurs avant de fermer le programme.

Le code est entièrement compatible avec un audit de fuites via Valgrind :

```bash
valgrind --leak-check=full --show-leak-kinds=all ./mypstree

```

*Résultat attendu : `All heap blocks were freed -- no leaks are possible`.*

```

```