#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <cassert>

using namespace std;

struct Submission {
    string problem;
    string status;
    int time;

    Submission(string p, string s, int t) : problem(p), status(s), time(t) {}
};

struct ProblemStatus {
    int attempts = 0;
    int solved_time = -1;
    int submissions_after_freeze = 0;
    bool is_frozen = false;
    bool is_solved = false;
};

struct Team {
    string name;
    int solved_count = 0;
    int total_penalty = 0;
    vector<int> solve_times;
    map<string, ProblemStatus> problems;
    vector<Submission> submissions;

    Team() = default;
    Team(string n) : name(n) {}
};

class ICPCManager {
private:
    bool competition_started = false;
    bool competition_ended = false;
    bool is_frozen = false;
    int duration_time = 0;
    int problem_count = 0;
    map<string, Team> teams;
    vector<string> team_order;

    vector<Team*> getRankedTeams() {
        vector<Team*> ranked;
        for (auto& [name, team] : teams) {
            ranked.push_back(&team);
        }

        sort(ranked.begin(), ranked.end(), [this](Team* a, Team* b) {
            if (!this->competition_started) {
                return a->name < b->name;
            }

            if (a->solved_count != b->solved_count) {
                return a->solved_count > b->solved_count;
            }

            if (a->total_penalty != b->total_penalty) {
                return a->total_penalty < b->total_penalty;
            }

            // Compare solve times in descending order
            for (size_t i = 0; i < min(a->solve_times.size(), b->solve_times.size()); i++) {
                if (a->solve_times[i] != b->solve_times[i]) {
                    return a->solve_times[i] < b->solve_times[i];
                }
            }

            // Lexicographic order of team names
            return a->name < b->name;
        });

        return ranked;
    }

    void printScoreboard(const vector<Team*>& ranked_teams) {
        for (Team* team : ranked_teams) {
            cout << team->name << " ";

            // Find ranking
            int rank = 1;
            for (int i = 0; i < ranked_teams.size(); i++) {
                if (ranked_teams[i] == team) {
                    rank = i + 1;
                    break;
                }
            }
            cout << rank << " ";

            cout << team->solved_count << " " << team->total_penalty << " ";

            // Print problem status
            for (int i = 0; i < problem_count; i++) {
                char prob = 'A' + i;
                string prob_str(1, prob);

                if (team->problems.find(prob_str) == team->problems.end()) {
                    cout << ".";
                } else {
                    const ProblemStatus& ps = team->problems[prob_str];
                    if (ps.is_frozen) {
                        if (ps.attempts == 0) {
                            cout << "0/" << ps.submissions_after_freeze;
                        } else {
                            cout << "-" << ps.attempts << "/" << ps.submissions_after_freeze;
                        }
                    } else if (ps.is_solved) {
                        if (ps.attempts == 0) {
                            cout << "+";
                        } else {
                            cout << "+" << ps.attempts;
                        }
                    } else {
                        if (ps.attempts == 0) {
                            cout << ".";
                        } else {
                            cout << "-" << ps.attempts;
                        }
                    }
                }

                if (i < problem_count - 1) cout << " ";
            }

            cout << "\n";
        }
    }

public:
    void addTeam(const string& team_name) {
        if (competition_started) {
            cout << "[Error]Add failed: competition has started.\n";
            return;
        }
        if (teams.find(team_name) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
            return;
        }
        teams[team_name] = Team(team_name);
        team_order.push_back(team_name);
        cout << "[Info]Add successfully.\n";
    }

    void startCompetition(int duration, int problems) {
        if (competition_started) {
            cout << "[Error]Start failed: competition has started.\n";
            return;
        }
        competition_started = true;
        duration_time = duration;
        problem_count = problems;
        cout << "[Info]Competition starts.\n";
    }

    void submit(const string& problem, const string& team_name,
                const string& status, int time) {
        if (teams.find(team_name) == teams.end()) return;

        Team& team = teams[team_name];
        team.submissions.emplace_back(problem, status, time);

        ProblemStatus& prob_status = team.problems[problem];

        if (!prob_status.is_solved) {
            if (status == "Accepted") {
                if (!is_frozen || !prob_status.is_frozen) {
                    // Normal case - not frozen or already solved before freeze
                    prob_status.is_solved = true;
                    prob_status.solved_time = time;
                    team.solved_count++;
                    team.total_penalty += 20 * prob_status.attempts + time;
                    team.solve_times.push_back(time);
                    sort(team.solve_times.rbegin(), team.solve_times.rend());
                } else {
                    // Problem is frozen - don't update stats yet
                    prob_status.submissions_after_freeze++;
                }
            } else {
                if (!is_frozen || !prob_status.is_frozen) {
                    prob_status.attempts++;
                } else {
                    prob_status.submissions_after_freeze++;
                }
            }
        }

        if (is_frozen && !prob_status.is_solved && prob_status.solved_time == -1) {
            prob_status.is_frozen = true;
        }
    }

    void flush() {
        cout << "[Info]Flush scoreboard.\n";
        auto ranked = getRankedTeams();
        printScoreboard(ranked);
    }

    void freeze() {
        if (!competition_started || competition_ended) return;
        if (is_frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            return;
        }
        is_frozen = true;
        cout << "[Info]Freeze scoreboard.\n";
    }

    void scroll() {
        if (!is_frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }

        cout << "[Info]Scroll scoreboard.\n";

        // First flush the scoreboard
        auto ranked_before = getRankedTeams();
        printScoreboard(ranked_before);

        // Keep track of original rankings
        map<Team*, int> original_rank;
        for (int i = 0; i < ranked_before.size(); i++) {
            original_rank[ranked_before[i]] = i + 1;
        }

        // Collect all frozen problems
        vector<pair<Team*, string>> frozen_problems;
        for (Team* team : ranked_before) {
            for (auto& [prob, status] : team->problems) {
                if (status.is_frozen) {
                    frozen_problems.emplace_back(team, prob);
                }
            }
        }

        // Process frozen problems one by one
        while (!frozen_problems.empty()) {
            // Find the lowest-ranked team with frozen problems
            Team* lowest_team = nullptr;
            int lowest_rank = 0;

            for (auto& [team, prob] : frozen_problems) {
                int rank = original_rank[team];
                if (!lowest_team || rank > lowest_rank) {
                    lowest_team = team;
                    lowest_rank = rank;
                }
            }

            // Find the smallest problem ID for this team
            string smallest_prob;
            for (auto& [team, prob] : frozen_problems) {
                if (team == lowest_team) {
                    if (smallest_prob.empty() || prob < smallest_prob) {
                        smallest_prob = prob;
                    }
                }
            }

            // Remove this problem from frozen list
            frozen_problems.erase(
                remove_if(frozen_problems.begin(), frozen_problems.end(),
                         [&](const auto& p) { return p.first == lowest_team && p.second == smallest_prob; }),
                frozen_problems.end()
            );

            // Unfreeze this problem
            lowest_team->problems[smallest_prob].is_frozen = false;

            // Check if this unfreezing causes any accepted submissions to be revealed
            bool has_accepted = false;

            // Look for accepted submissions to this problem that occurred after freezing
            for (auto& sub : lowest_team->submissions) {
                if (sub.problem == smallest_prob && sub.status == "Accepted" &&
                    lowest_team->problems[smallest_prob].solved_time == -1) {
                    // This was an accepted submission that wasn't counted yet
                    lowest_team->problems[smallest_prob].solved_time = sub.time;
                    lowest_team->problems[smallest_prob].is_solved = true;
                    lowest_team->solved_count++;
                    lowest_team->total_penalty += 20 * lowest_team->problems[smallest_prob].attempts + sub.time;
                    lowest_team->solve_times.push_back(sub.time);
                    sort(lowest_team->solve_times.rbegin(), lowest_team->solve_times.rend());
                    has_accepted = true;
                }
            }

            if (has_accepted) {
                // Check if ranking changed
                auto ranked_after = getRankedTeams();

                // Find teams whose ranking changed
                map<Team*, int> new_rank;
                for (int i = 0; i < ranked_after.size(); i++) {
                    new_rank[ranked_after[i]] = i + 1;
                }

                // Check if the team that just got an accepted submission moved up
                int old_pos = original_rank[lowest_team];
                int new_pos = new_rank[lowest_team];

                if (new_pos < old_pos) {
                    // This team moved up
                    Team* moved_up = lowest_team;
                    Team* moved_down = nullptr;

                    // Find which team was displaced
                    for (int i = 0; i < min(ranked_before.size(), ranked_after.size()); i++) {
                        if (ranked_before[i] != ranked_after[i]) {
                            if (ranked_after[i] == moved_up) {
                                moved_down = ranked_before[i];
                                break;
                            }
                        }
                    }

                    if (moved_down && moved_down != moved_up) {
                        cout << moved_up->name << " " << moved_down->name << " "
                             << moved_up->solved_count << " " << moved_up->total_penalty << "\n";
                    }
                }

                ranked_before = ranked_after;
                original_rank = new_rank;
            }
        }

        // Print final scoreboard
        printScoreboard(ranked_before);

        is_frozen = false;
    }

    void queryRanking(const string& team_name) {
        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query ranking.\n";
        if (is_frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }

        auto ranked = getRankedTeams();
        int ranking = 0;
        for (int i = 0; i < ranked.size(); i++) {
            if (ranked[i]->name == team_name) {
                ranking = i + 1;
                break;
            }
        }

        cout << "[" << team_name << "] NOW AT RANKING " << ranking << "\n";
    }

    void querySubmission(const string& team_name, const string& problem,
                        const string& status) {
        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query submission.\n";

        Team& team = teams[team_name];
        Submission* found = nullptr;

        // Search backwards for the last matching submission
        for (int i = team.submissions.size() - 1; i >= 0; i--) {
            Submission& sub = team.submissions[i];
            bool match_problem = (problem == "ALL" || sub.problem == problem);
            bool match_status = (status == "ALL" || sub.status == status);

            if (match_problem && match_status) {
                found = &sub;
                break;
            }
        }

        if (!found) {
            cout << "Cannot find any submission.\n";
        } else {
            cout << "[" << team_name << "] [" << found->problem << "] ["
                 << found->status << "] [" << found->time << "]\n";
        }
    }

    void end() {
        competition_ended = true;
        cout << "[Info]Competition ends.\n";
    }
};

int main() {
    ICPCManager manager;
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;

        istringstream iss(line);
        string command;
        iss >> command;

        if (command == "ADDTEAM") {
            string team_name;
            iss >> team_name;
            manager.addTeam(team_name);
        } else if (command == "START") {
            string dummy;
            int duration, problems;
            iss >> dummy >> duration >> dummy >> problems;
            manager.startCompetition(duration, problems);
        } else if (command == "SUBMIT") {
            string problem, team, status, dummy;
            int time;
            iss >> problem >> dummy >> team >> dummy >> status >> dummy >> time;
            manager.submit(problem, team, status, time);
        } else if (command == "FLUSH") {
            manager.flush();
        } else if (command == "FREEZE") {
            manager.freeze();
        } else if (command == "SCROLL") {
            manager.scroll();
        } else if (command == "QUERY_RANKING") {
            string team_name;
            iss >> team_name;
            manager.queryRanking(team_name);
        } else if (command == "QUERY_SUBMISSION") {
            string team_name, dummy, problem, status;
            iss >> team_name >> dummy >> dummy >> problem >> dummy >> status;
            manager.querySubmission(team_name, problem, status);
        } else if (command == "END") {
            manager.end();
            break;
        }
    }

    return 0;
}