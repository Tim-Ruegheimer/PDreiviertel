float getFilteredAverage() {
  int count = 5;
  int recentValues[5];

  // Letzte 5 Werte holen (mit Ringpuffer-Logik)
  for (int i = 0; i < count; i++) {
    int index = (currentIndex - 1 - i + storageSize) % storageSize;
    recentValues[i] = values[index];
  }

  // Min und Max finden
  int minVal = recentValues[0];
  int maxVal = recentValues[0];
  int sum = recentValues[0];

  for (int i = 1; i < count; i++) {
    if (recentValues[i] < minVal) minVal = recentValues[i];
    if (recentValues[i] > maxVal) maxVal = recentValues[i];
    sum += recentValues[i];
  }

  // Summe minus min und max
  int filteredSum = sum - minVal - maxVal;

  // Mittelwert aus 3 Werten berechnen
  float average = filteredSum / 3.0;

  return average;
}