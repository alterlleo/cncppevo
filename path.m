%% Author: Leonardo Pasquato
% Brief: compare script which plots both the ttrajectories with enabled and disabled TRC
% It's possible to check:
%   - 2D trajectory
%   - 3D trajectory
%   - Feedrate

%% Loading csv log

log_trc = 'logtrc.csv';
log_notrc = 'lognotrc.csv';

% Load files
opts = detectImportOptions(log_trc);
opts = setvartype(opts, {'n', 'type'}, 'string');
trc = readtable(log_trc, opts);
notrc = readtable(log_notrc, opts);

trc = sortrows(trc, 't_tot');
notrc = sortrows(notrc, 't_tot');

% Extract x, Y, Z, feedrate, total time
x_trc = trc.X;
y_trc = trc.Y;
z_trc = trc.Z;
x_notrc = notrc.X;
y_notrc = notrc.Y;
z_notrc = notrc.Z;
f_trc = trc.feedrate;
f_notrc = notrc.feedrate;
t_trc = trc.t_tot;
t_notrc = notrc.t_tot;

%% 2D plot
figure;
hold on; grid on; axis equal;
xlabel('X [mm]');
ylabel('Y [mm]');
title('CNCPPevo trajectory comparison (2D) between TRC / no TRC');
plot(x_notrc, y_notrc, 'r--', 'LineWidth', 1.5);
plot(x_trc,   y_trc,   'b-',  'LineWidth', 1.5);
legend('no TRC', 'TRC', 'Location', 'best');

%% 3D plot
figure;
hold on; grid on; axis equal;
xlabel('X [mm]');
ylabel('Y [mm]');
zlabel('Z [mm]');
title('CNCPPevo trajectory comparison (3D) between TRC / no TRC');
plot3(x_notrc, y_notrc, z_notrc, 'r--', 'LineWidth', 1.5);
plot3(x_trc,   y_trc,   z_trc,   'b-',  'LineWidth', 1.5);
legend('no TRC', 'TRC', 'Location', 'best');
view(3);

%% Feedrate plot
figure;
hold on; grid on;
xlabel('Total time [s]');
ylabel('Feedrate [mm/min]');
title('CNCPPevo feedrate comparison between TRC / no TRC');
plot(t_notrc, f_notrc, 'r--', 'LineWidth', 1.5);
plot(t_trc,   f_trc,   'b-',  'LineWidth', 1.5);
legend('no TRC', 'TRC', 'Location', 'best');
