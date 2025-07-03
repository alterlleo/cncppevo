# Author: Leonardo Pasquato
# Brief: compare script which plots both trajectories with enabled and disabled TRC
# Features:
#   - 2D trajectory
#   - 3D trajectory
#   - Feedrate

library(readr)
library(dplyr)
library(ggplot2)
library(plotly)

log_trc <- '~/Desktop/Workspace/cncppevo/logtrc.csv'
log_notrc <- '~/Desktop/Workspace/cncppevo/lognotrc.csv'

trc <- read_csv(log_trc, col_types = cols(n = col_character(), type = col_character()))
notrc <- read_csv(log_notrc, col_types = cols(n = col_character(), type = col_character()))

trc <- trc %>% arrange(t_tot)
notrc <- notrc %>% arrange(t_tot)

p2d <- ggplot() +
  geom_path(data = notrc, aes(x = X, y = Y), color = "red", linetype = "dashed", size = 1) +
  geom_path(data = trc, aes(x = X, y = Y), color = "blue", size = 1) +
  labs(
    title = "CNCPPevo trajectory comparison (2D) between TRC / no TRC",
    x = "X [mm]",
    y = "Y [mm]"
  ) +
  coord_equal() +
  theme_minimal()

print(p2d)

p3d <- plot_ly() %>%
  add_trace(data = notrc, x = ~X, y = ~Y, z = ~Z,
            type = 'scatter3d', mode = 'lines',
            line = list(color = 'red', dash = 'dash'),
            name = 'no TRC') %>%
  add_trace(data = trc, x = ~X, y = ~Y, z = ~Z,
            type = 'scatter3d', mode = 'lines',
            line = list(color = 'blue'),
            name = 'TRC') %>%
  layout(
    title = "CNCPPevo trajectory comparison (3D) between TRC / no TRC",
    scene = list(
      xaxis = list(title = "X [mm]"),
      yaxis = list(title = "Y [mm]"),
      zaxis = list(title = "Z [mm]")
    )
  )

p3d

p_feed <- ggplot() +
  geom_line(data = notrc, aes(x = t_tot, y = feedrate), color = "red", linetype = "dashed", size = 1) +
  geom_line(data = trc, aes(x = t_tot, y = feedrate), color = "blue", linewidth = 1) +
  labs(
    title = "CNCPPevo feedrate comparison between TRC / no TRC",
    x = "Total time [s]",
    y = "Feedrate [mm/min]"
  ) +
  theme_minimal()

print(p_feed)

