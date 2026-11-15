#include "charts.h"
#include "util/str.h"

#include <algorithm>
#include <cmath>
#include <numeric>

using namespace ftxui;

namespace gui {

Stats computeStats(std::vector<double> v) {
    Stats st;
    if (v.empty()) return st;
    st.count = v.size();
    st.mean = std::accumulate(v.begin(), v.end(), 0.0) / v.size();
    if (v.size() > 1) {
        double vs = 0.0;
        for (double x : v) vs += (x - st.mean) * (x - st.mean);
        st.std_dev = std::sqrt(vs / (v.size() - 1));
    }
    std::sort(v.begin(), v.end());
    st.min = v.front();
    st.max = v.back();
    size_t n = v.size();
    st.median = (n % 2 == 1) ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2.0;
    return st;
}

std::vector<SubjectStats> computeSubjectStats(const std::vector<Student>& students) {
    std::vector<SubjectStats> out;
    for (size_t j = 0; j < SUBJECTS.size(); ++j) {
        std::vector<double> vals;
        for (const auto& s : students)
            if (j < s.grades.size()) vals.push_back(s.grades[j]);
        out.push_back({SUBJECTS[j], computeStats(std::move(vals))});
    }
    return out;
}

namespace {

Element barRow(const std::string& label, double frac, const std::string& value,
               Color c, int labelWidth = 16) {
    frac = std::clamp(frac, 0.0, 1.0);
    return hbox({
        text(label) | size(WIDTH, EQUAL, labelWidth),
        gauge(static_cast<float>(frac)) | color(c) | flex,
        text(" " + value) | size(WIDTH, EQUAL, 9),
    });
}

Element rangeRuler(const std::string& label, const Stats& st, Color c) {
    const int W = 48;
    double span = st.max - st.min;
    auto pos = [&](double v) {
        if (span <= 0.0) return 0;
        int p = static_cast<int>(std::lround((v - st.min) / span * (W - 1)));
        return std::clamp(p, 0, W - 1);
    };
    std::vector<std::string> cells(W, "─");
    int a = pos(std::max(st.min, st.mean - st.std_dev));
    int b = pos(std::min(st.max, st.mean + st.std_dev));
    for (int i = a; i <= b; ++i) cells[i] = "━";
    cells[pos(st.median)] = "◆";
    cells[pos(st.mean)] = (pos(st.mean) == pos(st.median)) ? "◉" : "●";
    std::string line;
    for (const auto& cell : cells) line += cell;
    return hbox(text(label) | size(WIDTH, EQUAL, 14), text(util::fmt(st.min)), text(" "),
                text(line) | color(c), text(" "), text(util::fmt(st.max)));
}

}

Element summaryStatsView(const std::vector<Student>& students) {
    auto all = computeSubjectStats(students);
    auto cell = [](const std::string& s) { return text(s) | size(WIDTH, EQUAL, 12); };

    Elements rows;
    rows.push_back(hbox({
        text("Subject") | bold | size(WIDTH, EQUAL, 14),
        text("Count")   | bold | size(WIDTH, EQUAL, 8),
        text("Mean")    | bold | size(WIDTH, EQUAL, 12),
        text("StdDev")  | bold | size(WIDTH, EQUAL, 12),
        text("Min")     | bold | size(WIDTH, EQUAL, 12),
        text("Max")     | bold | size(WIDTH, EQUAL, 12),
        text("Median")  | bold | size(WIDTH, EQUAL, 12),
    }));
    rows.push_back(separator());

    Elements rulers;
    for (const auto& ss : all) {
        const auto& st = ss.stats;
        auto v = [&](double x) { return st.count ? util::fmt(x) : std::string("-"); };
        rows.push_back(hbox({
            text(ss.subject)               | size(WIDTH, EQUAL, 14),
            text(std::to_string(st.count)) | size(WIDTH, EQUAL, 8),
            cell(v(st.mean)),
            cell(v(st.std_dev)),
            cell(v(st.min)),
            cell(v(st.max)),
            cell(v(st.median)),
        }));
        if (st.count) rulers.push_back(rangeRuler(ss.subject, st, Color::Cyan));
    }
    rulers.push_back(text("● mean  ◆ median  ◉ both  ━ within 1 std dev") | dim);

    return vbox({
        vbox(std::move(rows)),
        text(""),
        text("Score range") | bold,
        vbox(std::move(rulers)),
    }) | vscroll_indicator | yframe;
}

Element subjectMeansChart(const std::vector<Student>& students) {
    auto all = computeSubjectStats(students);
    Elements rows;
    for (const auto& ss : all) {
        if (ss.stats.count == 0) {
            rows.push_back(barRow(ss.subject, 0.0, "-", Color::GrayDark, 14));
            continue;
        }
        rows.push_back(barRow(ss.subject, ss.stats.mean / 100.0,
                              util::fmt(ss.stats.mean), Color::Cyan, 14));
    }
    return vbox(std::move(rows));
}

Element averageDistributionChart(const std::vector<Student>& students) {
    std::vector<double> avgs;
    avgs.reserve(students.size());
    for (const auto& s : students) avgs.push_back(studentAverage(s));

    if (avgs.empty()) return text("No data.") | dim;

    Stats st = computeStats(avgs);
    size_t n = avgs.size();
    int bins = (st.max == st.min)
        ? 1 : std::clamp(static_cast<int>(std::ceil(std::sqrt(static_cast<double>(n)))), 3, 10);
    double width = (st.max == st.min) ? 1.0 : (st.max - st.min) / bins;
    std::vector<int> counts(bins, 0);
    for (double v : avgs) {
        int b = (bins == 1) ? 0
            : std::min(bins - 1, static_cast<int>((v - st.min) / width));
        counts[b]++;
    }
    int maxCount = 1;
    for (int c : counts) maxCount = std::max(maxCount, c);

    Elements rows;
    for (int b = 0; b < bins; ++b) {
        std::string label = (bins == 1) ? util::fmt(st.min, 1)
            : util::fmt(st.min + b * width, 1) + " - " + util::fmt(st.min + (b + 1) * width, 1);
        rows.push_back(barRow(label, static_cast<double>(counts[b]) / maxCount,
                              std::to_string(counts[b]), Color::Cyan, 22));
    }
    return vbox(std::move(rows));
}

}