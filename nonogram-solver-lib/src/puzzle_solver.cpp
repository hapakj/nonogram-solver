#include "puzzle_solver.h"

#include <iostream>
#include <functional>
#include <optional>
#include <iomanip>


bool PuzzleSolver::Solve()
{
	auto apply_rule = [&](auto&& rule)
	{
		for (size_t i = 0; i < m_puzzle.GetRowCount(); i++)
		{
			m_processed_row_id = i;
			rule(m_puzzle.GetRowValues(i), m_puzzle.GetRowView(i));
		}
		m_processed_row_id.reset();

		for (size_t i = 0; i < m_puzzle.GetColumnCount(); i++)
		{
			m_processed_column_id = i;
			rule(m_puzzle.GetColumnValues(i), m_puzzle.GetColumnView(i));
		}
		m_processed_column_id.reset();
	};

	auto last_state = m_puzzle;

	for (;;)
	{
		std::cout << (m_iteration_count + 1) << ". iteration" << std::endl;
		log() << (m_iteration_count + 1) << ". iteration" << std::endl;

		{
			log() << "Apply Marker rule" << std::endl;

			apply_rule([&](auto values, auto gridview) { MarkerRule(values, gridview); });

			m_puzzle.Print(log());
			log() << std::endl << std::endl << std::flush;
		}

		{
			log() << "Apply Zero Value rule" << std::endl;

			apply_rule([&](auto values, auto gridview) { ZeroValueRule(values, gridview); });

			m_puzzle.Print(log());
			log() << std::endl << std::endl << std::flush;
		}

		{
			log() << "Apply Close Side rule" << std::endl;

			apply_rule([&](auto values, auto gridview) { CloseSideRule(values, gridview); });

			m_puzzle.Print(log());
			log() << std::endl << std::endl << std::flush;
		}

		if (Puzzle::IsGridEqual(m_puzzle, last_state))
		{
			std::cout << "No change" << std::endl;
			break;
		}

		log() << std::endl << std::endl << std::flush;

		last_state = m_puzzle;
		m_iteration_count++;
	}

	{
		m_puzzle.Print(std::cout);
		std::cout << std::setprecision(2) << std::fixed << "Completion ratio: " << m_puzzle.GetCompletionRatio() * 100.0 << "%" << std::endl;
	}

	{
		log() << std::endl << std::endl << std::endl << "Final result:" << std::endl;

		m_puzzle.Print(log());
		log() << std::setprecision(2) << std::fixed << "Completion ratio: " << m_puzzle.GetCompletionRatio() * 100.0 << "%" << std::endl;
	}

	return true;
}


void PuzzleSolver::MarkerRule(const std::vector<int32_t>& values, Puzzle::GridView& grid_view)
{
	struct SectionRange
	{
		size_t begin{ 0 }, end{ 0 };
	};

	std::vector<SectionRange> section_ranges;

	{
		std::optional<SectionRange> section;

		for (size_t id = 0; id < grid_view.size(); id++)
		{
			if ((grid_view.at(id) != Puzzle::FieldState::Empty) && !section)
			{
				section.emplace();
				section->begin = id;
			}

			const size_t next_id = id + 1;

			if (section && ((next_id == grid_view.size()) || (grid_view.at(next_id) == Puzzle::FieldState::Empty))) 
			{
				section->end = next_id;
				section_ranges.push_back(*section);
				section.reset();
			}
		}
	}

	for (const auto section_range : section_ranges)
	{
		struct SectionView
		{
		public:
			SectionView(Puzzle::GridView& grid_view, const SectionRange& section_range)
				: m_grid_view(grid_view)
				, m_section_range(section_range)
			{
			}

			auto size()
			{
				return m_section_range.end - m_section_range.begin;
			}

			auto at(size_t id)
			{
				return m_grid_view.at(m_section_range.begin + id);
			}

			void set(size_t id, Puzzle::FieldState state)
			{
				m_grid_view.set(m_section_range.begin + id, state);
			}

		private:
			Puzzle::GridView& m_grid_view;
			SectionRange m_section_range;
		};


		struct SectionValueView
		{
		public:
			SectionValueView(const std::vector<int32_t>& values, size_t start, size_t end)
				: m_values(values)
				, m_start(start)
				, m_end(end)
			{
			}

			auto begin()
			{
				return m_values.begin() + m_start;
			}

			auto end()
			{
				return m_values.begin() + m_end;
			}

			auto size()
			{
				return m_end - m_start;
			}

			auto& operator[](size_t id)
			{
				return m_values[m_start + id];
			}

		private:
			const std::vector<int32_t>& m_values;
			size_t m_start{ 0 }, m_end{ 0 };
		};


		size_t start_value_id{ 0 };
		{
			size_t s = 0;
			size_t i = 0;

			for (; i < values.size(); i++)
			{
				s += values[i] + 1;
				if (s > section_range.begin)
				{
					break;
				}
			}

			start_value_id = i;
		}


		size_t end_value_id{ 0 };
		{
			size_t s = grid_view.size();
			int32_t i = int32_t(values.size()) - 1;

			for (; i >= 0; i--)
			{
				s -= values[i] + 1;
				if (s < section_range.end)
				{
					break;
				}
			}

			end_value_id = i + 1;
		}

		if (end_value_id < start_value_id)
		{
			continue;
		}

		SectionView section_view(grid_view, section_range);
		SectionValueView section_values(values, start_value_id, end_value_id);

		for (size_t i = 0; i < section_values.size(); i++)
		{
			const auto section_value = section_values[i];

			uint32_t left_padding{ 0 };
			for (size_t j = 0; j < i; j++)
			{
				left_padding += section_values[j] + 1;
			}

			uint32_t right_padding{ 0 };
			for (size_t j = i + 1; j < section_values.size(); j++)
			{
				right_padding += section_values[j] + 1;
			}

			auto right_end = left_padding + section_value;
			auto left_begin = section_view.size() - (right_padding + section_value);

			for (size_t j = left_begin; j < right_end; j++)
			{
				section_view.set(j, Puzzle::FieldState::Marked);
			}
		}
	}
}


void PuzzleSolver::ZeroValueRule(const std::vector<int32_t>& values, Puzzle::GridView& grid_view)
{
	bool zero_value = (values.size() == 1) && (values[0] == 0);

	if (!zero_value)
	{
		return;
	}

	for (size_t i = 0; i < grid_view.size(); i++)
	{
		grid_view.set(i, Puzzle::FieldState::Empty);
	}
}


void PuzzleSolver::CloseSideRule(const std::vector<int32_t>& values, Puzzle::GridView& grid_view)
{
	size_t first_unknown_id = 0;
	for (size_t i = 0; i < grid_view.size(); i++)
	{
		if (grid_view.at(i) == Puzzle::FieldState::Unknown)
		{
			first_unknown_id = i;
			break;
		}
	}

	if (first_unknown_id == grid_view.size())
	{
		return;
	}

	if (first_unknown_id == 0)
	{
		return;
	}

	if (grid_view.at(first_unknown_id - 1) != Puzzle::FieldState::Marked)
	{
		return;
	}

	size_t value_id = 0;
	for (size_t i = 0; i < first_unknown_id; i++)
	{
		if ((grid_view.at(i) == Puzzle::FieldState::Marked) && (grid_view.at(i + 1) == Puzzle::FieldState::Empty))
		{
			value_id++;
		}
	}

	size_t side_begin = 0;
	for (size_t i = first_unknown_id; i > 1; i--)
	{
		if (grid_view.at(i - 1) == Puzzle::FieldState::Empty)
		{
			side_begin = i;
			break;
		}
	}

	for (int32_t i = 0; i < values[value_id]; i++)
	{
		grid_view.set(side_begin + i, Puzzle::FieldState::Marked);
	}

	auto empty_id = side_begin + values[value_id];
	if (empty_id < grid_view.size())
	{
		grid_view.set(empty_id, Puzzle::FieldState::Empty);
	}
}

