% Plot F-measure curve

F_files = dir(fullfile('Data\\result_F', '*'));
F_names = {F_files.name};

id_figure = 0;
for idx=1:numel(F_names)
	F_name = F_names{idx};
	if (strcmp(F_name, '..') || strcmp(F_name, '.') || strcmp(F_name, 'desktop.ini'))
		continue;
    end
	
    id_figure = id_figure + 1;
    
	A = load(fullfile('Data\\result_F', F_name));
    n_predict = A(1:end, 1);
    f_measure = A(1:end, 2);

    fig = figure('visible', 'off');
	plot(n_predict, f_measure, 'Marker', '.');
    title(F_name);
    xlabel('N');
    ylabel('F-measure');
    print(fig, fullfile('Data\\result_F', strcat(F_name, '.png')), '-dpng');
end
