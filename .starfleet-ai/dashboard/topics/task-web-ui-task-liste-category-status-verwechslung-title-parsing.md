Title: "Web-UI Task-Liste: Category/Status-Verwechslung + Title-Parsing"
Category: active
Kind: task
Status: "assigned"
Created-By: "Enterprise"
Created: "2026-09-15T09:56:36Z"
Assigned-To: "Enterprise"
Doc-Ref: "—"
Slug: task-web-ui-task-liste-category-status-verwechslung-title-parsing

Bugs in der Task-Liste (renderTasks in index.html):

1. **Category = Status-Verwechslung:** Tasks in Unterverzeichnissen parked/done/analysis bekommen den Verzeichnisnamen als Category-Prefix ([parked], [done], [analysis]) — das sind aber auch gültige Status-Werte. Verwirrend: Prefix sieht nach Status aus, ist aber Kategorie.
   Fix: parked/done/analysis als Category ignorieren oder den eigentlichen Slug-Teil (z.B. xlibre/task-x) als Prefix anzeigen.

2. **'active' als Default-Category (29 Tasks):** Tasks in topics/ (top-level) bekommen Category='active' als Default. Anzeige [active] Title ist uninformativ.
   Fix: Category leer lassen wenn default, oder bessere Kategorie aus slug ableiten.

3. **Leere Title (2 Tasks):** desqview/... und xlibre/container-build-e2e-complete haben leeren Title → Fallback auf slug. Edit-Formular zeigt leeres Subject.
   Fix: Title-Sanitization oder Pflichtfeld beim Capture.

4. **Optische Trennung:** [category] ist aktuell innerhalb des &lt;a&gt;-Links. Soll nicht klickbar sein, nur der Titel.
   Fix: &lt;span class='task-cat'&gt;[cat]&lt;/span&gt; &lt;a class='task-title-link'&gt;title&lt;/a&gt;.
