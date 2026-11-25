// Typing Practice Game - OOP capstone (C++17)
// Native:  g++ -std=c++17 -Wall -o typing_practice typing_practice.cpp
// Web:     em++ typing_practice.cpp -std=c++17 -O2 -lembind -s MODULARIZE=1 -s EXPORT_NAME=createModule -o typing_practice.js
#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#ifndef __EMSCRIPTEN__
#include <chrono>
#endif

// ---------- SentenceBank: owns the data ----------
class SentenceBank {
    std::vector<std::string> sentences_{
        "The quick brown fox jumps over the lazy dog.",
        "Object oriented design models real world things as objects.",
        "A good programmer writes code that humans can read.",
        "Practice every day and your fingers will remember the keys.",
        "Encapsulation hides the details and exposes a simple interface.",
        "Small functions that do one job are easy to test and reuse."};
public:
    const std::vector<std::string>& all() const { return sentences_; }
};

// ---------- Result of one attempt ----------
struct Result {
    int correct = 0;       // characters matching the target at the same position
    int typed = 0;         // characters the user typed
    double accuracy = 0;   // percent
    double wpm = 0;        // words per minute (1 word = 5 characters)
};

// ---------- TextComparer: the core mechanism ----------
class TextComparer {
public:
    // Position-by-position comparison.
    static int countCorrect(const std::string& target, const std::string& typed) {
        int n = 0;
        size_t len = std::min(target.size(), typed.size());
        for (size_t i = 0; i < len; ++i) n += (target[i] == typed[i]);
        return n;
    }
    // Accuracy = correct / max(target, typed): missing and extra characters both count against you.
    static double accuracy(int correct, size_t targetLen, size_t typedLen) {
        size_t denom = std::max(targetLen, typedLen);
        return denom ? 100.0 * correct / denom : 0.0;
    }
    // Net WPM = (correct characters / 5) / minutes.
    static double wpm(int correct, double seconds) {
        return seconds > 0 ? (correct / 5.0) / (seconds / 60.0) : 0.0;
    }
};

// ---------- TypingGame: rounds and totals (no I/O, no clock: time is passed in) ----------
class TypingGame {
    std::vector<std::string> queue_;
    std::string current_;
    Result last_;
    int rounds_ = 0;
    double sumAcc_ = 0, sumWpm_ = 0, bestWpm_ = 0;
    std::mt19937 rng_{std::random_device{}()};
public:
    explicit TypingGame(const SentenceBank& bank, int rounds = 3) {
        queue_ = bank.all();
        std::shuffle(queue_.begin(), queue_.end(), rng_);
        if ((int)queue_.size() > rounds) queue_.resize(rounds);
    }
    bool nextSentence() {
        if (queue_.empty()) return false;
        current_ = queue_.back(); queue_.pop_back();
        return true;
    }
    const Result& submit(const std::string& typed, double seconds) {
        last_.correct = TextComparer::countCorrect(current_, typed);
        last_.typed = (int)typed.size();
        last_.accuracy = TextComparer::accuracy(last_.correct, current_.size(), typed.size());
        last_.wpm = TextComparer::wpm(last_.correct, seconds);
        ++rounds_; sumAcc_ += last_.accuracy; sumWpm_ += last_.wpm;
        bestWpm_ = std::max(bestWpm_, last_.wpm);
        return last_;
    }
    const std::string& sentence() const { return current_; }
    const Result& last() const { return last_; }
    int rounds() const { return rounds_; }
    double avgAccuracy() const { return rounds_ ? sumAcc_ / rounds_ : 0; }
    double avgWpm() const { return rounds_ ? sumWpm_ / rounds_ : 0; }
    double bestWpm() const { return bestWpm_; }
};

#ifndef __EMSCRIPTEN__
// ---------- Console front-end ----------
int main() {
    SentenceBank bank;
    TypingGame game(bank);
    std::cout << "=== TYPING PRACTICE ===\nType each sentence exactly and press Enter. The timer starts when it is shown.\n";
    while (game.nextSentence()) {
        std::cout << "\n" << game.sentence() << "\n> ";
        auto start = std::chrono::steady_clock::now();
        std::string typed;
        if (!std::getline(std::cin, typed)) break;
        double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        const Result& r = game.submit(typed, secs);
        std::cout.setf(std::ios::fixed); std::cout.precision(1);
        std::cout << "Accuracy: " << r.accuracy << "%   Speed: " << r.wpm << " WPM   Time: " << secs << "s\n";
    }
    std::cout.setf(std::ios::fixed); std::cout.precision(1);
    std::cout << "\nAverage accuracy: " << game.avgAccuracy() << "%  Average speed: " << game.avgWpm()
              << " WPM  Best: " << game.bestWpm() << " WPM\n";
}
#else
// ---------- WebAssembly bridge ----------
#include <emscripten/bind.h>
#include <memory>
class WebTyping {
    SentenceBank bank_;
    std::unique_ptr<TypingGame> game_;
public:
    WebTyping() : game_(new TypingGame(bank_)) {}
    bool nextSentence()                         { return game_->nextSentence(); }
    std::string sentence() const                { return game_->sentence(); }
    void submit(const std::string& t, double s) { game_->submit(t, s); }
    double accuracy() const { return game_->last().accuracy; }
    double wpm() const      { return game_->last().wpm; }
    double avgAccuracy() const { return game_->avgAccuracy(); }
    double avgWpm() const      { return game_->avgWpm(); }
    double bestWpm() const     { return game_->bestWpm(); }
};
EMSCRIPTEN_BINDINGS(typing_practice) {
    emscripten::class_<WebTyping>("WebTyping")
        .constructor<>()
        .function("nextSentence", &WebTyping::nextSentence)
        .function("sentence", &WebTyping::sentence)
        .function("submit", &WebTyping::submit)
        .function("accuracy", &WebTyping::accuracy)
        .function("wpm", &WebTyping::wpm)
        .function("avgAccuracy", &WebTyping::avgAccuracy)
        .function("avgWpm", &WebTyping::avgWpm)
        .function("bestWpm", &WebTyping::bestWpm);
}
#endif
